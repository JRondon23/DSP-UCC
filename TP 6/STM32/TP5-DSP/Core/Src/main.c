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
#include "adc.h"
#include "dac.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "FIR.h"
#include "iir_sos.h"
#include "iir_coeffs.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* Elegir uno de los cuatro y recompilar. */
#define FILTRO_FIR_PASABAJO     0
#define FILTRO_FIR_PASAALTO     1
#define FILTRO_IIR_PASABANDA    2
#define FILTRO_IIR_RECHAZABANDA 3
#ifndef FILTRO_ACTIVO
#define FILTRO_ACTIVO FILTRO_IIR_PASABANDA
#endif
#if FILTRO_ACTIVO < 0 || FILTRO_ACTIVO > 3
#error "FILTRO_ACTIVO invalido"
#endif
#if IIR_FS_HZ != 8000
#error "Ajustar TIM2 al cambiar la frecuencia de diseno"
#endif
#define ADC_VREF 3.3f
/* Ajustar al offset real del generador (por defecto 1.65 V). */
#define OFFSET_ENTRADA_V 1.65f
#define OFFSET_SALIDA_V  1.65f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t muestra = 0;
volatile float conv = 0.0f; /* Salida antes de sumar offset y limitar al DAC. */
volatile uint32_t dac_value = 0;
volatile uint32_t muestras_procesadas = 0;
volatile uint32_t errores_adc = 0;
volatile uint32_t saturaciones_dac = 0;
volatile uint32_t fallos_numericos = 0;
volatile uint32_t ciclos_filtro_max = 0;
#if FILTRO_ACTIVO == FILTRO_IIR_PASABANDA
static IIR_State estado[IIR_PASABANDA_SECTIONS] = {0};
#elif FILTRO_ACTIVO == FILTRO_IIR_RECHAZABANDA
static IIR_State estado[IIR_RECHAZABANDA_SECTIONS] = {0};
#else
static float input_signal[N_FIR] = {0};
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static float filtrar_muestra(float x)
{
#if FILTRO_ACTIVO == FILTRO_IIR_PASABANDA
    return iir_process(x, iir_pasabanda, estado, IIR_PASABANDA_SECTIONS);
#elif FILTRO_ACTIVO == FILTRO_IIR_RECHAZABANDA
    return iir_process(x, iir_rechazabanda, estado, IIR_RECHAZABANDA_SECTIONS);
#else
    for (int i = N_FIR - 1; i > 0; --i)
        input_signal[i] = input_signal[i - 1];
    input_signal[0] = x;
    float y = 0.0f;
    for (int i = 0; i < N_FIR; ++i) {
#if FILTRO_ACTIVO == FILTRO_FIR_PASABAJO
        y += fir_pasabajo[i] * input_signal[i];
#else
        y += fir_pasaalto[i] * input_signal[i];
#endif
    }
    return y;
#endif
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
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
    /* Iniciar el reloj de muestreo al final, con ADC y DAC preparados. */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
        Error_Handler();
    if (HAL_DAC_Start(&hdac1, DAC_CHANNEL_1) != HAL_OK)
        Error_Handler();
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048);
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
        Error_Handler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        /* ADC -> filtro -> DAC se procesa una vez por conversion en el callback. */
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
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 20;
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

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
            ++errores_adc;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance != ADC1)
        return;
    uint32_t inicio = DWT->CYCCNT;
    muestra = HAL_ADC_GetValue(hadc);
    float x = (float)muestra * ADC_VREF / 4095.0f - OFFSET_ENTRADA_V;
    conv = filtrar_muestra(x);
    float salida_v = conv + OFFSET_SALIDA_V;
    if (!isfinite(salida_v)) {
        ++fallos_numericos;
        salida_v = OFFSET_SALIDA_V;
    }
    if (salida_v < 0.0f) {
        salida_v = 0.0f;
        ++saturaciones_dac;
    } else if (salida_v > ADC_VREF) {
        salida_v = ADC_VREF;
        ++saturaciones_dac;
    }
    /* Limitar en float ANTES de convertir al entero sin signo. */
    dac_value = (uint32_t)(salida_v * 4095.0f / ADC_VREF + 0.5f);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac_value);
    ++muestras_procesadas;
    uint32_t ciclos = DWT->CYCCNT - inicio;
    if (ciclos > ciclos_filtro_max)
        ciclos_filtro_max = ciclos;
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
        ++errores_adc;
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
	while (1) {
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
