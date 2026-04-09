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
#include "cmsis_os2.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern osMessageQueueId_t CommandQueueHandle;
extern osMessageQueueId_t LEDStateQueueHandle;
extern osSemaphoreId_t BinarySemButtonHandle;
extern osSemaphoreId_t BinarySemUartHandle;
extern osEventFlagsId_t LedTriggerEventHandle;
extern osTimerId_t BreathLedTimerHandle;
extern osTimerId_t BrtLedStaTimerHandle; 
extern uint8_t Rx_Data;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void StartReceive_IT(UART_HandleTypeDef *huartx);
// 调试函数: 打印任务列表(含栈溢出检测)
void PrintTaskList(void);
// 检查指定任务的栈使用情况
void CheckTaskStack(const char *taskName);

#ifdef DEBUG_MEMORY
// 内存泄漏检测
void memory_stats_print(void);
#endif

#if USE_MEMORY_POOL
// 内存池初始化(可选优化方案)
void MemPool_Init(void);
#endif

// 旧的任务声明已废弃,保留仅用于兼容
// void StartvSerialHandlerTask(void *argument);
// void StartLedTask(void *argument);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Button_Pin GPIO_PIN_13
#define Button_GPIO_Port GPIOC
#define Button_EXTI_IRQn EXTI15_10_IRQn
#define ButtonWKUP_Pin GPIO_PIN_0
#define ButtonWKUP_GPIO_Port GPIOA
#define LEDG_Pin GPIO_PIN_0
#define LEDG_GPIO_Port GPIOB
#define USB_EN_Pin GPIO_PIN_3
#define USB_EN_GPIO_Port GPIOD
#define LEDR_Pin GPIO_PIN_5
#define LEDR_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
/* LED Trigger Event */
#define LED_TRIGGER_EVENT_SERIAL   (1 << 0)
#define LED_TRIGGER_EVENT_BUTTON   (1 << 1)
#define LED_TRIGGER_EVENT_WKUP     (1 << 2)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
