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

/*
 *
 * 	El codigo esta preparado para aplicar filtros FIR en su version flotante o punto fijo
 * 	lo unico que debe hacerse es comentar aquello que no se va a usar (cosas de float si se quiere punto fijo y
 * 	al reves). Si se recorre el codigo vera que tiene comentarios cuando se esten inicializando las variables y funciones
 * 	de punto fijo o de punto flotante. Para que se vea mas claro, las cosas tipo q_X son de punto fijo y las de
 * 	floatX_t son de punto flotante
 *
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
#include <math.h>
#include "arm_math.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

/* USER CODE BEGIN PV */

/*
 *
 * Inicializo variables para CMSIS float
 *
 */

arm_fir_instance_f32 FIR; //Creo una instancia FIR float
float32_t Buffer_estado[N_FIR + 1]; //Por seguridad le doy +1


/*
 *
 * Inicializo variables para CMSIS Punto Fijo
 *
 */

//arm_fir_instance_q15 FIR; //Creo una instancia FIR Q15 (punto fijo)
//q15_t Buffer_estado[N_FIR + 1];

/*----------------------------*/

volatile uint8_t adc_ready = 0; //variable general.

/*
 *
 * 	Variables para FIR FLOAT
 *
 */
float32_t muestra = 0;

float32_t conv = 0;

/*
 *
 * 	Variables para FIR Punto flotante
 *
 */
//q15_t muestra = 0;
//q15_t conv = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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

	/*
	 *
	 * Inicializo el FIR FLOAT
	 *
	 *	Analizando los parametros en orden:
	 *	@&FIR, es el objeto tipo FIR creado
	 *	@N_FIR, es la cantidad de estados
	 *	@fir_pasabajo, son los coeficientes del filtro casteados a float32
	 *	@Buffer_estado, es el buffer (espacio de memoria) que le asigno para que trabaje el filtro
	 *	@1, es el tamaño del bloque que trabaja, lo pongo en 1 porque trabajamos muestra a muestra
	 */

	arm_fir_init_f32(&FIR, N_FIR, (float32_t*) fir_pasabajo, Buffer_estado, 1);

	/*
	 *
	 * Inicializo el FIR Punto Fijo
	 *
	 *	Analizando los parametros en orden:
	 *	@&FIR, es el objeto tipo FIR creado
	 *	@N_FIR, es la cantidad de estados
	 *	@fir_pasabajo, son los coeficientes del filtro casteados a float32
	 *	@Buffer_estado, es el buffer (espacio de memoria) que le asigno para que trabaje el filtro
	 *	@1, es el tamaño del bloque que trabaja, lo pongo en 1 porque trabajamos muestra a muestra
	 */

	//arm_fir_init_q15(&FIR, N_FIR, (float32_t*) fir_pasabajo, Buffer_estado, 1);
	/*------------------------------------------------------------------------------*/

	HAL_TIM_Base_Start_IT(&htim2);
	HAL_ADC_Start_IT(&hadc1);
	HAL_DAC_Start(&hdac1, DAC1_CHANNEL_1);

	/* USER CODE END 2 */

	/* Initialize led */
	BSP_LED_Init(LED_GREEN);

	/* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
	BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

	/* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
	BspCOMInit.BaudRate = 115200;
	BspCOMInit.WordLength = COM_WORDLENGTH_8B;
	BspCOMInit.StopBits = COM_STOPBITS_1;
	BspCOMInit.Parity = COM_PARITY_NONE;
	BspCOMInit.HwFlowCtl = COM_HWCONTROL_NONE;
	if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE) {
		Error_Handler();
	}

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		if (adc_ready == 1) {

			/*
			 *
			 * FIR float
			 *
			 */
			arm_fir_f32(&FIR, &muestra, &conv, 1);

			/*
			 *
			 * FIR Punto fijo
			 *
			 */
			//arm_fir_q15(&FIR, &muestra, &conv, 1);

			uint32_t valor_dac = (uint32_t) (conv * 4095.0f / 3.3f);

			HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, valor_dac); //le asigno valor
			HAL_DAC_Start(&hdac1, DAC_CHANNEL_1); //Pongo a convertir.

			adc_ready = 0;
		}

	}

	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
	RCC_OscInitStruct.PLL.PLLN = 8;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
	RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

	if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {

		HAL_ADC_Start_IT(&hadc1);

		adc_ready = 1;
	}

}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {

	if (hadc->Instance == ADC1) {
//	uint8_t i;
//	for (i = N_FIR; i > 0; --i) {
//		input_signal[i] = input_signal[i - 1];
//	}
		muestra = HAL_ADC_GetValue(hadc) * 3.3f / 4095.0; //es directamente la muestra la que nos llega

//	input_signal[0] = muestra * 3.3f / 4095.0;

	}

}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
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
