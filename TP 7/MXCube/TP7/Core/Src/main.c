/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tp7_fir_coeffs.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* Un buffer tiene un solo propietario: el ADC o el bucle principal. */
typedef enum
{
  TP7_BUFFER_FREE = 0,
  TP7_BUFFER_FILLING,
  TP7_BUFFER_READY,
  TP7_BUFFER_PROCESSING,
  TP7_BUFFER_PLAYING
} TP7_BufferState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Paso 1: TIM8 sera el reloj de muestreo del TP7, sin DMA ni TRGO. */
#define TP7_SAMPLE_RATE_HZ 8000U
/* Codigo central de un conversor de 12 bits: representa aproximadamente
 * 1,65 V con Vref=3,3 V. Se usa hasta disponer de muestras procesadas. */
#define TP7_DAC_MIDPOINT 2048U
#define TP7_BLOCK_SIZE 128U
#define TP7_INPUT_BUFFER_COUNT 2U
#define TP7_OUTPUT_BUFFER_COUNT 2U
/* Dos bloques de espera al arrancar: 256/8000 = 32 ms. Este margen permite
 * preparar cada salida en el while sin exigir que termine en solo 125 us. */
#define TP7_STARTUP_SAMPLES (2U * TP7_BLOCK_SIZE)
#define TP7_CONV_SIZE (TP7_BLOCK_SIZE + TP7_FIR_TAPS - 1U)
/* Esta implementacion almacena toda la cola en el siguiente bloque. */
#if (TP7_FIR_TAPS - 1U) > TP7_BLOCK_SIZE
#error "La cola FIR debe caber en un bloque; ajustar el algoritmo si cambia el orden."
#endif
#if TP7_SAMPLE_RATE_HZ != TP7_FIR_FS_HZ
#error "El reloj de muestreo debe coincidir con el diseno del FIR."
#endif
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

DAC_HandleTypeDef hdac1;

TIM_HandleTypeDef htim8;

/* USER CODE BEGIN PV */
/* Contador de diagnostico: puede observarse en el depurador.
 * volatile indica que se modifica desde una interrupcion. */
volatile uint32_t timer_ticks = 0U;
/* Ultimo codigo entregado al DAC. TIM8 lo toma del bloque de salida activo,
 * o mantiene 2048 durante las primeras 256 muestras de arranque. */
volatile uint16_t dac_next_sample = TP7_DAC_MIDPOINT;
/* El ADC completa la conversion despues de que termina la rutina de TIM8.
 * pending evita iniciar otra adquisicion si la anterior aun no fue atendida.
 * last_sample se conserva como diagnostico ademas del almacenamiento en bloques. */
volatile uint16_t adc_last_sample = 0U;
volatile uint8_t adc_pending = 0U;
volatile uint32_t adc_samples = 0U;
/* Diagnostico si se entra en Error_Handler: 0=sin error, 1=ADC pendiente,
 * 2=fallo al escribir DAC, 3=fallo al iniciar ADC, 4=error del ADC,
 * 5=no hay un buffer libre para continuar la adquisicion,
 * 6=falta un bloque de salida al momento de reproducirlo.
 * No se imprime desde las interrupciones. */
volatile uint32_t sample_error = 0U;

/* Entrada ping-pong: ADC llena A mientras el while atiende B, y viceversa.
 * El primer buffer comienza FILLING y el segundo FREE. Solo el callback
 * del ADC modifica el indice de escritura y la posicion de la muestra.
 * Se usa volatile porque los datos se comparten con la interrupcion. */
static volatile uint16_t adc_buffers[TP7_INPUT_BUFFER_COUNT][TP7_BLOCK_SIZE];
static volatile TP7_BufferState input_state[TP7_INPUT_BUFFER_COUNT] =
  {TP7_BUFFER_FILLING, TP7_BUFFER_FREE};
static uint32_t adc_write_buffer = 0U;
static uint32_t adc_write_index = 0U;

/* Bloque privado del while, expresado en cuentas ADC centradas en cero.
 * Por ejemplo: 2048 -> 0.0f, 3072 -> 1024.0f. No se convierte a voltios.
 * El FIR opera sobre este bloque antes de volver a sumar 2048 para el DAC. */
static float input_block[TP7_BLOCK_SIZE];
/* Memoria privada del while: convolucion de 256 posiciones y cola de 128.
 * La cola arranca en cero y se conserva entre llamadas, nunca se borra al
 * comenzar un bloque nuevo. Las IRQ no acceden a estas dos memorias. */
static float fir_result[TP7_CONV_SIZE];
static float fir_tail[TP7_FIR_TAPS - 1U];
volatile uint32_t input_blocks_acquired = 0U;
volatile uint32_t input_blocks_prepared = 0U;

/* Salida ping-pong: FREE -> FILLING (while) -> READY -> PLAYING (TIM8) -> FREE.
 * Los indices de lectura y escritura avanzan alternando 0,1,0,1, para que un
 * bloque nuevo nunca adelante a otro pendiente de reproducirse. */
static volatile uint16_t dac_buffers[TP7_OUTPUT_BUFFER_COUNT][TP7_BLOCK_SIZE];
static volatile TP7_BufferState output_state[TP7_OUTPUT_BUFFER_COUNT] =
  {TP7_BUFFER_FREE, TP7_BUFFER_FREE};
static uint32_t dac_write_buffer = 0U; /* Solo lo modifica el while. */
static uint32_t dac_read_buffer = 0U;  /* Solo lo modifica TIM8. */
static uint32_t dac_read_index = 0U;
static uint32_t startup_remaining = TP7_STARTUP_SAMPLES;
volatile uint32_t output_blocks_prepared = 0U;
volatile uint32_t output_blocks_played = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC1_Init(void);
static void MX_TIM8_Init(void);
/* USER CODE BEGIN PFP */
static void TP7_PrepareInputBlock(void);
static uint16_t TP7_NextOutputSample(void);
static void TP7_FilterInputBlock(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Convolucion temporal por bloques con superponer-sumar (overlap-add).
 * 128 entradas * 129 coeficientes producen 256 valores de convolucion.
 * Solo se ejecuta desde el while, siempre con las IRQ habilitadas. */
static void TP7_FilterInputBlock(void)
{
  /* 1. El resultado de ESTE bloque empieza en cero; la cola se conserva. */
  for (uint32_t i = 0U; i < TP7_CONV_SIZE; i++)
  {
    fir_result[i] = 0.0f;
  }

  /* 2. Cada entrada contribuye a 129 posiciones de la salida.
   * Indice maximo: 127+128=255, dentro de las 256 posiciones reservadas. */
  for (uint32_t n = 0U; n < TP7_BLOCK_SIZE; n++)
  {
    for (uint32_t k = 0U; k < TP7_FIR_TAPS; k++)
    {
      fir_result[n + k] += input_block[n] * fir_h[k];
    }
  }

  /* 3. Sumar la contribucion pendiente del bloque anterior. Asi el filtro
   * no se reinicia en cada frontera: la salida es una convolucion continua. */
  for (uint32_t i = 0U; i < TP7_FIR_TAPS - 1U; i++)
  {
    fir_result[i] += fir_tail[i];
  }

  /* 4. Las primeras 128 salidas se enviaran al DAC. Guardar las ultimas 128
   * para la proxima llamada; no convertir ni limitar la cola a cuentas DAC. */
  for (uint32_t i = 0U; i < TP7_FIR_TAPS - 1U; i++)
  {
    fir_tail[i] = fir_result[TP7_BLOCK_SIZE + i];
  }
}

/* Toma un bloque READY sin permitir que el ADC lo modifique durante la copia.
 * Las IRQ se suspenden solo para cambiar la propiedad, nunca durante las
 * 128 conversiones a float. Se restaura PRIMASK, no se habilitan IRQ a ciegas. */
static void TP7_PrepareInputBlock(void)
{
  uint32_t selected = TP7_INPUT_BUFFER_COUNT;
  uint32_t output = dac_write_buffer;
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  for (uint32_t b = 0U; b < TP7_INPUT_BUFFER_COUNT; b++)
  {
    if ((input_state[b] == TP7_BUFFER_READY) &&
        (output_state[output] == TP7_BUFFER_FREE))
    {
      input_state[b] = TP7_BUFFER_PROCESSING;
      output_state[output] = TP7_BUFFER_FILLING;
      selected = b;
      break;
    }
  }
  __DMB();
  __set_PRIMASK(primask);

  if (selected == TP7_INPUT_BUFFER_COUNT)
  {
    /* No hay entrada completa o aun no se libero la salida correspondiente.
     * Reintentar en la siguiente vuelta sin consumir ni descartar la entrada. */
    return;
  }

  for (uint32_t n = 0U; n < TP7_BLOCK_SIZE; n++)
  {
    input_block[n] = (float)adc_buffers[selected][n]
                         - (float)TP7_DAC_MIDPOINT;
  }

  /* La copia centrada ya no depende del buffer crudo: el ADC puede reutilizarlo.
   * La convolucion usa la copia privada, con las interrupciones habilitadas. */
  primask = __get_PRIMASK();
  __disable_irq();
  __DMB();
  input_state[selected] = TP7_BUFFER_FREE;
  __set_PRIMASK(primask);

  /* Procesar el bloque una sola vez, antes de publicar su salida. */
  TP7_FilterInputBlock();

  /* Convertir la salida FIR centrada a codigos DAC: agregar el nivel central,
   * limitar al rango de 12 bits y redondear. La saturacion solo afecta al DAC,
   * nunca al estado interno ni a la cola del filtro. */
  for (uint32_t n = 0U; n < TP7_BLOCK_SIZE; n++)
  {
    float code = fir_result[n] + (float)TP7_DAC_MIDPOINT;
    if (code < 0.0f) { code = 0.0f; }
    else if (code > 4095.0f) { code = 4095.0f; }
    dac_buffers[output][n] = (uint16_t)(code + 0.5f);
  }

  /* Publicar el bloque SOLO al terminar las 128 escrituras. TIM8 no puede
   * reproducir un buffer FILLING. Mantener la seccion critica muy corta. */
  primask = __get_PRIMASK();
  __disable_irq();
  __DMB();
  output_state[output] = TP7_BUFFER_READY;
  dac_write_buffer ^= 1U;
  input_blocks_prepared++;
  output_blocks_prepared++;
  __set_PRIMASK(primask);
}

/* Solo TIM8 llama esta funcion: consume exactamente una muestra por tick.
 * No calcula el filtro ni recorre el bloque entero dentro de la IRQ. */
static uint16_t TP7_NextOutputSample(void)
{
  if (startup_remaining != 0U)
  {
    startup_remaining--;
    return TP7_DAC_MIDPOINT;
  }

  if (dac_read_index == 0U)
  {
    if (output_state[dac_read_buffer] != TP7_BUFFER_READY)
    {
      sample_error = 6U;
      Error_Handler();
      return TP7_DAC_MIDPOINT;
    }
    output_state[dac_read_buffer] = TP7_BUFFER_PLAYING;
  }

  uint16_t code = dac_buffers[dac_read_buffer][dac_read_index];
  dac_read_index++;
  if (dac_read_index == TP7_BLOCK_SIZE)
  {
    /* La ultima muestra ya esta copiada en code: el while puede reutilizar
     * el buffer sin modificar lo que se escribira al DAC en este tick. */
    __DMB();
    output_state[dac_read_buffer] = TP7_BUFFER_FREE;
    dac_read_buffer ^= 1U;
    dac_read_index = 0U;
    output_blocks_played++;
  }
  return code;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_DAC1_Init();
  MX_TIM8_Init();
  /* USER CODE BEGIN 2 */
  /* Preparacion ANTES del reloj de muestreo: calibrar el ADC y dejar el DAC
   * en su nivel central. Estas operaciones solo se realizan al arrancar. */
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R,
                       TP7_DAC_MIDPOINT) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  /* La IRQ del ADC no esta habilitada en el .ioc actual. Se habilita aqui
   * por software. ADC1_2_IRQHandler, en stm32g4xx_it.c, atiende la conversion.
   * Misma prioridad que TIM8: las dos rutinas no se interrumpen entre si. */
  HAL_NVIC_SetPriority(ADC1_2_IRQn, 0U, 0U);
  HAL_NVIC_ClearPendingIRQ(ADC1_2_IRQn);
  HAL_NVIC_EnableIRQ(ADC1_2_IRQn);

  /* Arranque por software: MX_TIM8_Init configura el timer, pero no lo inicia.
   * La inicializacion puede dejar UIF pendiente; se borra para que la primera
   * interrupcion ocurra despues de un periodo completo de 125 us. */
  __HAL_TIM_SET_COUNTER(&htim8, 0U);
  __HAL_TIM_CLEAR_FLAG(&htim8, TIM_FLAG_UPDATE);
  if (HAL_TIM_Base_Start_IT(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* Atiende un bloque completo si lo hay. No espera muestras en un bucle
     * bloqueante: ADC y TIM8 siguen funcionando por interrupciones. */
    TP7_PrepareInputBlock();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT2 config
  */
  sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
  sConfig.DAC_DMADoubleDataMode = DISABLE;
  sConfig.DAC_SignedFormat = DISABLE;
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 19;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 999;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim8, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : LPUART1_TX_Pin LPUART1_RX_Pin */
  GPIO_InitStruct.Pin = LPUART1_TX_Pin|LPUART1_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF12_LPUART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

//No se debe filtrar un bloque completo ni imprimir dentro de esta funcion.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM8)
  {
    timer_ticks++;

    /* 1. Una conversion debe terminar y ser atendida antes del siguiente tick.
     * Si no ocurre, detenerse permite detectar el fallo sin perder muestras
     * silenciosamente ni iniciar dos conversiones para el mismo intervalo. */
    if (adc_pending != 0U)
    {
      sample_error = 1U;
      Error_Handler();
      return;
    }

    /* 2. Publicar una muestra YA PREPARADA del bloque activo. Como no usamos
     * trigger ni DMA, la escritura actualiza la salida directamente. */
    dac_next_sample = TP7_NextOutputSample();
    if (HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R,
                         dac_next_sample) != HAL_OK)
    {
      sample_error = 2U;
      Error_Handler();
      return;
    }

    /* 3. Iniciar UNA conversion por software y salir sin esperar su resultado.
     * Se marca pending antes de habilitar la conversion. La IRQ del ADC
     * guardara la lectura y liberara la marca cuando finalice. */
    adc_pending = 1U;
    if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
    {
      sample_error = 3U;
      Error_Handler();
    }
  }
}

/* Guarda UNA muestra por conversion. Al completar 128, entrega el bloque
 * al while y cambia al otro buffer. No convierte a float ni filtra en la IRQ. */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    adc_last_sample = (uint16_t)HAL_ADC_GetValue(hadc);
    adc_samples++;

    adc_buffers[adc_write_buffer][adc_write_index] =
      adc_last_sample;
    adc_write_index++;

    if (adc_write_index == TP7_BLOCK_SIZE)
    {
      uint32_t next = adc_write_buffer ^ 1U;

      /* La siguiente adquisicion necesita el otro buffer. Si sigue READY
       * o PROCESSING, el while se atraso: detenerse antes de sobrescribirlo.
       * Con fs=8000 Hz, dispone aproximadamente de 16 ms entre bloques. */
      if (input_state[next] != TP7_BUFFER_FREE)
      {
        sample_error = 5U;
        Error_Handler();
        return;
      }

      __DMB(); /* Publicar READY despues de escribir todas las muestras. */
      input_state[adc_write_buffer] = TP7_BUFFER_READY;
      input_state[next] = TP7_BUFFER_FILLING;
      adc_write_buffer = next;
      adc_write_index = 0U;
      input_blocks_acquired++;
    }

    adc_pending = 0U;
  }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc->Instance == ADC1)
  {
    sample_error = 4U;
    Error_Handler();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
