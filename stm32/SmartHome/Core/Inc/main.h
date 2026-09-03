/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BUZZER_Pin GPIO_PIN_13
#define BUZZER_GPIO_Port GPIOC
#define KEY_UP_Pin GPIO_PIN_0
#define KEY_UP_GPIO_Port GPIOA
#define LIGHT_LED_Pin GPIO_PIN_0
#define LIGHT_LED_GPIO_Port GPIOB
#define OLED_SCL_Pin GPIO_PIN_12
#define OLED_SCL_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_13
#define OLED_SDA_GPIO_Port GPIOB
#define DHT11_DATA_Pin GPIO_PIN_14
#define DHT11_DATA_GPIO_Port GPIOB
#define STEPPER_IN1_Pin GPIO_PIN_11
#define STEPPER_IN1_GPIO_Port GPIOA
#define STEPPER_IN2_Pin GPIO_PIN_12
#define STEPPER_IN2_GPIO_Port GPIOA
#define STEPPER_IN3_Pin GPIO_PIN_15
#define STEPPER_IN3_GPIO_Port GPIOA
#define STEPPER_IN4_Pin GPIO_PIN_3
#define STEPPER_IN4_GPIO_Port GPIOB
#define KEY_1_Pin GPIO_PIN_5
#define KEY_1_GPIO_Port GPIOB
#define KEY_2_Pin GPIO_PIN_8
#define KEY_2_GPIO_Port GPIOB
#define KEY_0_Pin GPIO_PIN_9
#define KEY_0_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
