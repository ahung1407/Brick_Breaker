/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "software_timer.h"
#include "lcd.h"
#include "ds3231.h"
#include "button.h"
#include "picture.h"
#include "game_ui.h"
#include "game_logic.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define INIT_SYSTEM     0
#define SET_HOUR        1
#define SET_MINUTE      2
#define SET_DAY         3
#define SET_DATE        4
#define SET_MONTH       5
#define SET_YEAR        6
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
GameState game_state;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void system_init();
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
	MX_TIM2_Init();
	MX_SPI1_Init();
	MX_FSMC_Init();
	MX_I2C1_Init();
	MX_TIM4_Init();
	/* USER CODE BEGIN 2 */
	system_init();
	timer2_set(20); // ~50 FPS ~ 20ms
	game_state.status = GAME_START_SCREEN;
	// Display Intro Screen (Background Image + "PRESS BUTTON 1 TO PLAY")
	lcd_show_picture(0, 0, 240, 320, gImage_BK);
	lcd_show_string_center(0, 164, "PRESS BUTTON 1 TO PLAY", WHITE, 0, 16, 1);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		button_scan();

		switch (game_state.status) {
		case GAME_START_SCREEN:
			if (button_count[0] == 1) { // Change from Intro to Playing Screen
				game_init_state(&game_state);
				game_state.show_potentiometer_prompt = 1;
				game_state.status = GAME_PLAYING;
				game_draw_initial_scene(&game_state);
			}
			break;
		case GAME_PLAYING:
			if (!game_state.show_potentiometer_prompt && timer2_flag == 1) { // Game Update over ~50 FPS
				step_world(&game_state, 0.02f); // Assuming dt = 0.02 seconds for ~50 FPS
				timer2_flag = 0;
				// debug
			    char vx_str[16];
			    char vy_str[16];

			    snprintf(vx_str, sizeof(vx_str), "%d", game_state.ball.dx);
			    snprintf(vy_str, sizeof(vy_str), "%d", game_state.ball.dy);
			    lcd_show_string_center(0, 164 - 8, vx_str, WHITE, 0, 16, 1);
			    lcd_show_string_center(0, 164 + 8, vy_str, WHITE, 0, 16, 1);
			    //end debug
				game_update_screen(&game_state); // only updates changed components like paddle  and ball
			}
			if (game_state.show_potentiometer_prompt && button_count[2] == 1) { // Start Game after showing prompt, 
																				// use potentiometer check  in the future
				game_state.show_potentiometer_prompt = 0;
				initialize_ball_velocity(&game_state.ball);		
				game_draw_initial_scene(&game_state);
			}

			if (button_count[4] == 1) { // Pause Button
				game_state.status = GAME_PAUSED;
				game_draw_pause_screen(&game_state);
			} else if (button_count[5] == 1) { // Game Over Button
				game_state.status = GAME_OVER;
				game_draw_game_over_screen(&game_state);
			}
			break;
		case GAME_PAUSED:
			if (button_count[4] == 1) { // Resume Button
				game_state.status = GAME_PLAYING;
				game_draw_initial_scene(&game_state);
			}
			break;
		case GAME_OVER:
			if (button_count[5] == 1) { // Restart Game from Game Over
				game_init_state(&game_state);
				game_state.status = GAME_PLAYING;
				game_state.show_potentiometer_prompt = 1;
				game_draw_initial_scene(&game_state);
			}
			break;
		default:
			break;
		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
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
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV4;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
void system_init() {
	HAL_GPIO_WritePin(OUTPUT_Y0_GPIO_Port, OUTPUT_Y0_Pin, 0);
	HAL_GPIO_WritePin(OUTPUT_Y1_GPIO_Port, OUTPUT_Y1_Pin, 0);
	HAL_GPIO_WritePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin, 0);

	lcd_init();
	ds3231_init();

	timer2_init();
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

#ifdef  USE_FULL_ASSERT
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
