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

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for HidTask */
osThreadId_t HidTaskHandle;
const osThreadAttr_t HidTask_attributes = {
  .name = "HidTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UartTask */
osThreadId_t UartTaskHandle;
const osThreadAttr_t UartTask_attributes = {
  .name = "UartTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ButtonTask */
osThreadId_t ButtonTaskHandle;
const osThreadAttr_t ButtonTask_attributes = {
  .name = "ButtonTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CommandDispTask */
osThreadId_t CommandDispTaskHandle;
const osThreadAttr_t CommandDispTask_attributes = {
  .name = "CommandDispTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for RespoMangerTask */
osThreadId_t RespoMangerTaskHandle;
const osThreadAttr_t RespoMangerTask_attributes = {
  .name = "RespoMangerTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for CommandQueue */
osMessageQueueId_t CommandQueueHandle;
const osMessageQueueAttr_t CommandQueue_attributes = {
  .name = "CommandQueue"
};
/* Definitions for ResponseQueue */
osMessageQueueId_t ResponseQueueHandle;
const osMessageQueueAttr_t ResponseQueue_attributes = {
  .name = "ResponseQueue"
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
/* Definitions for BinarySemUart */
osSemaphoreId_t BinarySemUartHandle;
const osSemaphoreAttr_t BinarySemUart_attributes = {
  .name = "BinarySemUart"
};
/* Definitions for LedTriggerEvent */
osEventFlagsId_t LedTriggerEventHandle;
const osEventFlagsAttr_t LedTriggerEvent_attributes = {
  .name = "LedTriggerEvent"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void HidAdapterTask(void *argument);
void UartAdapterTask(void *argument);
void ButtonAdapterTask(void *argument);
void CommandDispatcherTask(void *argument);
void ResponseManagerTask(void *argument);
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
  // 初始化各模块
  LedController_Init();
  SystemMonitor_Init();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of BinarySemButton */
  BinarySemButtonHandle = osSemaphoreNew(1, 0, &BinarySemButton_attributes);

  /* creation of BinarySemUart */
  BinarySemUartHandle = osSemaphoreNew(1, 0, &BinarySemUart_attributes);

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
  //CommandQueueHandle = osMessageQueueNew(32, sizeof(Command_t), &CommandQueue_attributes);

  /* creation of ResponseQueue - 统一响应队列 */
  //ResponseQueueHandle = osMessageQueueNew(16, sizeof(Response_t), &ResponseQueue_attributes);

  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of CommandQueue */
  CommandQueueHandle = osMessageQueueNew (32, sizeof(Command_t *), &CommandQueue_attributes);

  /* creation of ResponseQueue */
  ResponseQueueHandle = osMessageQueueNew (16, sizeof(Response_t *), &ResponseQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  HidInputAdapter_Init(CommandQueueHandle);
  UartInputAdapter_Init(CommandQueueHandle);
  ButtonInputAdapter_Init(CommandQueueHandle);
  CommandDispatcher_Init(CommandQueueHandle, ResponseQueueHandle);
  ResponseManager_Init(ResponseQueueHandle);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of HidTask */
  HidTaskHandle = osThreadNew(HidAdapterTask, NULL, &HidTask_attributes);

  /* creation of UartTask */
  UartTaskHandle = osThreadNew(UartAdapterTask, NULL, &UartTask_attributes);

  /* creation of ButtonTask */
  ButtonTaskHandle = osThreadNew(ButtonAdapterTask, NULL, &ButtonTask_attributes);

  /* creation of CommandDispTask */
  CommandDispTaskHandle = osThreadNew(CommandDispatcherTask, NULL, &CommandDispTask_attributes);

  /* creation of RespoMangerTask */
  RespoMangerTaskHandle = osThreadNew(ResponseManagerTask, NULL, &RespoMangerTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of LedTriggerEvent */
  LedTriggerEventHandle = osEventFlagsNew(&LedTriggerEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_HidAdapterTask */
/**
  * @brief  Function implementing the HidTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_HidAdapterTask */
__weak void HidAdapterTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN HidAdapterTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END HidAdapterTask */
}

/* USER CODE BEGIN Header_UartAdapterTask */
/**
* @brief Function implementing the UartTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_UartAdapterTask */
__weak void UartAdapterTask(void *argument)
{
  /* USER CODE BEGIN UartAdapterTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END UartAdapterTask */
}

/* USER CODE BEGIN Header_ButtonAdapterTask */
/**
* @brief Function implementing the ButtonTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ButtonAdapterTask */
__weak void ButtonAdapterTask(void *argument)
{
  /* USER CODE BEGIN ButtonAdapterTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ButtonAdapterTask */
}

/* USER CODE BEGIN Header_CommandDispatcherTask */
/**
* @brief Function implementing the CommandDispTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_CommandDispatcherTask */
__weak void CommandDispatcherTask(void *argument)
{
  /* USER CODE BEGIN CommandDispatcherTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END CommandDispatcherTask */
}

/* USER CODE BEGIN Header_ResponseManagerTask */
/**
* @brief Function implementing the RespoMangerTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ResponseManagerTask */
__weak void ResponseManagerTask(void *argument)
{
  /* USER CODE BEGIN ResponseManagerTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ResponseManagerTask */
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

// 调试辅助函数: 打印所有运行中的任务
#include "task.h"

#ifdef DEBUG_MEMORY
#include <stdlib.h>

typedef struct {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    uint32_t timestamp;
} MemAllocRecord_t;

#define MAX_ALLOC_RECORDS 100
static MemAllocRecord_t alloc_records[MAX_ALLOC_RECORDS];
static uint16_t record_count = 0;
static uint32_t total_allocated = 0;
static uint32_t total_freed = 0;

/**
  * @brief  调试版内存分配
  */
void* debug_malloc(size_t size, const char *file, int line)
{
    void *ptr = pvPortMalloc(size);
    
    if (ptr != NULL && record_count < MAX_ALLOC_RECORDS) {
        alloc_records[record_count].ptr = ptr;
        alloc_records[record_count].size = size;
        alloc_records[record_count].file = file;
        alloc_records[record_count].line = line;
        alloc_records[record_count].timestamp = osKernelGetTickCount();
        record_count++;
        total_allocated += size;
        
        printf("[MEM] Alloc %d bytes at %p (%s:%d)\n", size, ptr, file, line);
    }
    
    return ptr;
}

/**
  * @brief  调试版内存释放
  */
void debug_free(void *ptr, const char *file, int line)
{
    if (ptr == NULL) {
        printf("[MEM] ❌ Attempt to free NULL pointer (%s:%d)\n", file, line);
        return;
    }
    
    // 查找记录
    int found = 0;
    for (uint16_t i = 0; i < record_count; i++) {
        if (alloc_records[i].ptr == ptr) {
            total_freed += alloc_records[i].size;
            // 清除记录
            alloc_records[i].ptr = NULL;
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printf("[MEM] ❌ Double free or invalid pointer at %p (%s:%d)\n", ptr, file, line);
    } else {
        printf("[MEM] Free %p (%s:%d)\n", ptr, file, line);
    }
    
    vPortFree(ptr);
}

/**
  * @brief  打印内存统计信息
  */
void memory_stats_print(void)
{
    printf("\n========== Memory Statistics ==========\n");
    printf("Total allocated: %lu bytes\n", total_allocated);
    printf("Total freed:     %lu bytes\n", total_freed);
    printf("Net allocated:   %lu bytes\n", total_allocated - total_freed);
    printf("Active records:  %d / %d\n", record_count, MAX_ALLOC_RECORDS);
    printf("Free heap:       %d bytes\n", xPortGetFreeHeapSize());
    
    // 检查未释放的内存
    uint16_t leak_count = 0;
    printf("\nUnfreed allocations:\n");
    for (uint16_t i = 0; i < record_count; i++) {
        if (alloc_records[i].ptr != NULL) {
            printf("  [%d] %p, %d bytes, %s:%d, tick=%lu\n",
                   leak_count++,
                   alloc_records[i].ptr,
                   alloc_records[i].size,
                   alloc_records[i].file,
                   alloc_records[i].line,
                   alloc_records[i].timestamp);
        }
    }
    
    if (leak_count == 0) {
        printf("  ✅ No memory leaks detected\n");
    } else {
        printf("  ⚠️⚠️⚠️ WARNING: %d memory leaks detected!\n", leak_count);
    }
    printf("========================================\n\n");
}
#endif

void PrintTaskList(void)
{
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime, ulStatsAsPercentage;
    
    // 获取任务数量
    uxArraySize = uxTaskGetNumberOfTasks();
    printf("\n=== FreeRTOS Task List (%ld tasks) ===\n", uxArraySize);
    
    // 分配状态数组
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        // 获取任务状态
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        
        printf("%-20s %-8s %-10s %-10s %s\n", "Task Name", "State", "Priority", "Stack HW", "Status");
        printf("------------------------------------------------------------------------\n");
        
        for (x = 0; x < uxArraySize; x++) {
            const char *pcStatus;
            const char *pcStackStatus = "OK";
            
            // 确定任务状态
            switch (pxTaskStatusArray[x].eCurrentState) {
                case eRunning:      pcStatus = "Running";   break;
                case eReady:        pcStatus = "Ready";     break;
                case eBlocked:      pcStatus = "Blocked";   break;
                case eSuspended:    pcStatus = "Suspended"; break;
                case eDeleted:      pcStatus = "Deleted";   break;
                default:            pcStatus = "Unknown";   break;
            }
            
            // ✅ 检查栈高水位标记,判断是否接近溢出
            uint16_t stackHW = pxTaskStatusArray[x].usStackHighWaterMark;
            if (stackHW < 50) {
                pcStackStatus = "⚠️ DANGER";  // 剩余<50字,极度危险
            } else if (stackHW < 100) {
                pcStackStatus = "⚠️ WARN";    // 剩余<100字,需要关注
            }
            
            // 计算栈高水位标记
            printf("%-20s %-8s %-10ld %-10d %s\n",
                   pxTaskStatusArray[x].pcTaskName,
                   pcStatus,
                   pxTaskStatusArray[x].uxCurrentPriority,
                   stackHW,
                   pcStackStatus);
        }
        
        vPortFree(pxTaskStatusArray);
        printf("========================================================================\n\n");
    } else {
        printf("[ERROR] Failed to allocate memory for task list\n");
    }
}

/**
  * @brief  检查指定任务的栈使用情况
  * @param  taskName: 任务名称
  */
void CheckTaskStack(const char *taskName)
{
    TaskHandle_t hTask = xTaskGetHandle(taskName);
    if (hTask != NULL) {
        uint16_t stackHW = uxTaskGetStackHighWaterMark(hTask);
        printf("[Stack Check] Task '%s': High Watermark = %d words (%d bytes)\n", 
               taskName, stackHW, stackHW * 4);
        
        if (stackHW < 50) {
            printf("  ⚠️⚠️⚠️ CRITICAL: Stack nearly exhausted! Increase stack size immediately!\n");
        } else if (stackHW < 100) {
            printf("  ⚠️ WARNING: Stack usage is high. Consider increasing stack size.\n");
        } else {
            printf("  ✅ Stack usage is normal.\n");
        }
    } else {
        printf("[ERROR] Task '%s' not found\n", taskName);
    }
}

/* USER CODE END Application */

