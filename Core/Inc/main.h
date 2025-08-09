/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RST_Pin GPIO_PIN_4
#define RST_GPIO_Port GPIOA
#define ILI_SCK_Pin GPIO_PIN_5
#define ILI_SCK_GPIO_Port GPIOA
#define ILI_MISO_Pin GPIO_PIN_6
#define ILI_MISO_GPIO_Port GPIOA
#define ILI_MOSI_Pin GPIO_PIN_7
#define ILI_MOSI_GPIO_Port GPIOA
#define CS_Pin GPIO_PIN_0
#define CS_GPIO_Port GPIOB
#define DC_Pin GPIO_PIN_1
#define DC_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_2
#define LED_GPIO_Port GPIOB
#define JUMP_BUTTON_Pin GPIO_PIN_12
#define JUMP_BUTTON_GPIO_Port GPIOB
#define RIGHT_BUTTON_Pin GPIO_PIN_13
#define RIGHT_BUTTON_GPIO_Port GPIOB
#define SD_CS_Pin GPIO_PIN_8
#define SD_CS_GPIO_Port GPIOA
#define LEFT_BUTTON_Pin GPIO_PIN_10
#define LEFT_BUTTON_GPIO_Port GPIOA
#define SD_Power_Pin GPIO_PIN_11
#define SD_Power_GPIO_Port GPIOA
#define FIRE_BUTTON_Pin GPIO_PIN_12
#define FIRE_BUTTON_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
#define SD_SPI_HANDLE hspi2
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
