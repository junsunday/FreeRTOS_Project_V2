/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "inputAdapters.h"
#include "CommandDispatcher.h"
#include "ResponseManager.h"
#include "LedController.h"
#include "SystemMonitor.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* Definitions for CommandQueue - 统一命令队列 */
osMessageQueueId_t CommandQueueHandle;
const osMessageQueueAttr_t CommandQueue_attributes = {
  .name = "CommandQueue"
};

/* Definitions for ResponseQueue - 统一响应队列 */
osMessageQueueId_t ResponseQueueHandle;
const osMessageQueueAttr_t ResponseQueue_attributes = {
  .name = "ResponseQueue"
};
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
// 旧的任务句柄已废弃,保留仅用于兼容
// static const char* Tasknames[] = {"defaultTask", "vSerialHandlerT", "vLedTask"};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for vSerialHandlerT */
osThreadId_t vSerialHandlerTHandle;
const osThreadAttr_t vSerialHandlerT_attributes = {
  .name = "vSerialHandlerT",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for vLedTask */
osThreadId_t vLedTaskHandle;
const osThreadAttr_t vLedTask_attributes = {
  .name = "vLedTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for vStatusMonitorT */
osThreadId_t vStatusMonitorTHandle;
const osThreadAttr_t vStatusMonitorT_attributes = {
  .name = "vStatusMonitorT",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};


/* Definitions for LEDStateQueue */
osMessageQueueId_t LEDStateQueueHandle;
const osMessageQueueAttr_t LEDStateQueue_attributes = {
  .name = "LEDStateQueue"
};
/* Definitions for BreathLedTimer */
osTimerId_t BreathLedTimerHandle;
const osTimerAttr_t BreathLedTimer_attributes = {
  .name = "BreathLedTimer"
};
/* Definitions for BrtLedStaTimer */
osTimerId_t BrtLedStaTimerHandle;
const osTimerAttr_t BrtLedStaTimer_attributes = {
  .name = "BrtLedStaTimer"
};
/* Definitions for BinarySemButton */
osSemaphoreId_t BinarySemButtonHandle;
const osSemaphoreAttr_t BinarySemButton_attributes = {
  .name = "BinarySemButton"
};
/* Definitions for LedTriggerEvent */
osEventFlagsId_t LedTriggerEventHandle;
const osEventFlagsAttr_t LedTriggerEvent_attributes = {
  .name = "LedTriggerEvent"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
// 新架构任务声明
void HidInputAdapter_Task(void *argument);
void UartInputAdapter_Task(void *argument);
void ButtonInputAdapter_Task(void *argument);
void CommandDispatcher_Task(void *argument);
void ResponseManager_Task(void *argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartvSerialHandlerTask(void *argument);
void StartLedTask(void *argument);
void StartStatusMonitorTask(void *argument);
void BreathLedCallback(void *argument);
void BrtLedStaCallback(void *argument);

extern void MX_USB_DEVICE_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of BinarySemButton */
  BinarySemButtonHandle = osSemaphoreNew(1, 0, &BinarySemButton_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of BreathLedTimer */
  BreathLedTimerHandle = osTimerNew(BreathLedCallback, osTimerPeriodic, NULL, &BreathLedTimer_attributes);

  /* creation of BrtLedStaTimer */
  BrtLedStaTimerHandle = osTimerNew(BrtLedStaCallback, osTimerOnce, NULL, &BrtLedStaTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* creation of CommandQueue - 统一命令队列 */
  CommandQueueHandle = osMessageQueueNew(32, sizeof(Command_t), &CommandQueue_attributes);

  /* creation of ResponseQueue - 统一响应队列 */
  ResponseQueueHandle = osMessageQueueNew(16, sizeof(Response_t), &ResponseQueue_attributes);

  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */


  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  

  // 业务逻辑层(通过命令分发器调用,无需独立任务)
  // LED控制器和系统监控器在CommandDispatcher中被调用
  
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
    // 输入适配器层任务
  /* creation of HidInputAdapter */
  osThreadNew(HidInputAdapter_Task, NULL, &(const osThreadAttr_t){.name = "HidInputAdapter", .stack_size = 256 * 4, .priority = osPriorityHigh});
  
  /* creation of UartInputAdapter */
  osThreadNew(UartInputAdapter_Task, NULL, &(const osThreadAttr_t){.name = "UartInputAdapter", .stack_size = 256 * 4, .priority = osPriorityHigh});
  
  /* creation of ButtonInputAdapter */
  osThreadNew(ButtonInputAdapter_Task, NULL, &(const osThreadAttr_t){.name = "ButtonInputAdapter", .stack_size = 128 * 4, .priority = osPriorityNormal});
  
  // 应用层任务
  /* creation of CommandDispatcher */
  osThreadNew(CommandDispatcher_Task, NULL, &(const osThreadAttr_t){.name = "CmdDispatcher", .stack_size = 256 * 4, .priority = osPriorityHigh});
  
  /* creation of ResponseManager */
  osThreadNew(ResponseManager_Task, NULL, &(const osThreadAttr_t){.name = "RespManager", .stack_size = 256 * 4, .priority = osPriorityHigh});
  
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of LedTriggerEvent - 保留用于兼容 */
  LedTriggerEventHandle = osEventFlagsNew(&LedTriggerEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
  
  /* USER CODE BEGIN InitModules */
  // 初始化各模块
  HidInputAdapter_Init(CommandQueueHandle);
  UartInputAdapter_Init(CommandQueueHandle);
  ButtonInputAdapter_Init(CommandQueueHandle);
  CommandDispatcher_Init(CommandQueueHandle, ResponseQueueHandle);
  ResponseManager_Init(ResponseQueueHandle);
  LedController_Init();
  SystemMonitor_Init();
  /* USER CODE END InitModules */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  * @note   此任务已被ButtonInputAdapter替代,保留仅用于兼容
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  // MX_USB_DEVICE_Init(); // 已在SystemMonitor_Init中调用
  
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    // 此任务已废弃,按钮处理由ButtonInputAdapter负责
    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartvSerialHandlerTask */
/**
* @brief Function implementing the vSerialHandlerT thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartvSerialHandlerTask */
__weak void StartvSerialHandlerTask(void *argument)
{
  /* USER CODE BEGIN StartvSerialHandlerTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartvSerialHandlerTask */
}

/* USER CODE BEGIN Header_StartLedTask */
/**
* @brief Function implementing the vLedTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLedTask */
__weak void StartLedTask(void *argument)
{
  /* USER CODE BEGIN StartLedTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartLedTask */
}

/* USER CODE BEGIN Header_StartStatusMonitorTask */
/**
* @brief Function implementing the vStatusMonitorT thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartStatusMonitorTask */
__weak void StartStatusMonitorTask(void *argument)
{
  /* USER CODE BEGIN StartStatusMonitorTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartStatusMonitorTask */
}

/* BreathLedCallback function */
__weak void BreathLedCallback(void *argument)
{
  /* USER CODE BEGIN BreathLedCallback */

  /* USER CODE END BreathLedCallback */
}

/* BrtLedStaCallback function */
__weak void BrtLedStaCallback(void *argument)
{
  /* USER CODE BEGIN BrtLedStaCallback */

  /* USER CODE END BrtLedStaCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

