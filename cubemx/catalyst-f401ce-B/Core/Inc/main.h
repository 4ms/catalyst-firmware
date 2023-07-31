/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ResetJack_Pin GPIO_PIN_13
#define ResetJack_GPIO_Port GPIOC
#define TrigIN_Pin GPIO_PIN_14
#define TrigIN_GPIO_Port GPIOC
#define enc1b_Pin GPIO_PIN_15
#define enc1b_GPIO_Port GPIOC
#define DEBUG1_Pin GPIO_PIN_2
#define DEBUG1_GPIO_Port GPIOA
#define MUXWRITE_Pin GPIO_PIN_3
#define MUXWRITE_GPIO_Port GPIOA
#define SEL0_Pin GPIO_PIN_6
#define SEL0_GPIO_Port GPIOA
#define SEL1_Pin GPIO_PIN_0
#define SEL1_GPIO_Port GPIOB
#define SEL2_Pin GPIO_PIN_1
#define SEL2_GPIO_Port GPIOB
#define MUXREAD0_Pin GPIO_PIN_2
#define MUXREAD0_GPIO_Port GPIOB
#define MUXREAD1_Pin GPIO_PIN_10
#define MUXREAD1_GPIO_Port GPIOB
#define enc8a_Pin GPIO_PIN_12
#define enc8a_GPIO_Port GPIOB
#define enc8b_Pin GPIO_PIN_13
#define enc8b_GPIO_Port GPIOB
#define enc2a_Pin GPIO_PIN_14
#define enc2a_GPIO_Port GPIOB
#define enc1a_Pin GPIO_PIN_15
#define enc1a_GPIO_Port GPIOB
#define enc7b_Pin GPIO_PIN_8
#define enc7b_GPIO_Port GPIOA
#define enc7a_Pin GPIO_PIN_9
#define enc7a_GPIO_Port GPIOA
#define enc6a_Pin GPIO_PIN_10
#define enc6a_GPIO_Port GPIOA
#define enc6b_Pin GPIO_PIN_11
#define enc6b_GPIO_Port GPIOA
#define enc5b_Pin GPIO_PIN_12
#define enc5b_GPIO_Port GPIOA
#define enc5a_Pin GPIO_PIN_15
#define enc5a_GPIO_Port GPIOA
#define enc4b_Pin GPIO_PIN_3
#define enc4b_GPIO_Port GPIOB
#define enc4a_Pin GPIO_PIN_4
#define enc4a_GPIO_Port GPIOB
#define enc3a_Pin GPIO_PIN_5
#define enc3a_GPIO_Port GPIOB
#define enc3B_Pin GPIO_PIN_6
#define enc3B_GPIO_Port GPIOB
#define enc2b_Pin GPIO_PIN_7
#define enc2b_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
