/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ButtonInputAdapter.c
  * @brief          : Button输入适配器实现
  ******************************************************************************
  */
/* USER CODE END Header */

#include "InputAdapters.h"
#include "CommandDef.h"
#include "main.h"
#include <stdio.h>

/* ========== 私有变量 ========== */
static osMessageQueueId_t s_cmdQueue = NULL;

/* ========== 公共函数实现 ========== */

/**
  * @brief  初始化Button输入适配器
  */
void ButtonInputAdapter_Init(osMessageQueueId_t cmdQueue)
{
    s_cmdQueue = cmdQueue;
}

/**
  * @brief  Button输入适配器任务
  */
void ButtonAdapterTask(void *argument)
{
    uint8_t lastState = GPIO_PIN_RESET;
    uint8_t currentState;
    
    for (;;) {
        // 读取按钮状态
        currentState = HAL_GPIO_ReadPin(ButtonWKUP_GPIO_Port, ButtonWKUP_Pin);
        
        // 检测上升沿(按下)
        if (currentState == GPIO_PIN_SET && lastState == GPIO_PIN_RESET) {
            // 消抖延迟
            osDelay(50);
            
            // 再次确认
            if (HAL_GPIO_ReadPin(ButtonWKUP_GPIO_Port, ButtonWKUP_Pin) == GPIO_PIN_SET) {
                // ⚠️ 从堆分配命令对象
                Command_t *cmd = CMD_MALLOC();
                if (cmd == NULL) {
                    printf("[Button Adapter] ❌ Memory allocation failed\n");
                } else {
                    // 构建命令
                    cmd->source = CMD_SRC_BUTTON;
                    cmd->command_id = CMD_LED_BREATH_START;
                    cmd->payload[0] = 0x01;
                    cmd->payload_len = 1;
                    cmd->timestamp = osKernelGetTickCount();
                    
                    // ✅ 发送指针到队列(所有权转移)
                    if (osMessageQueuePut(s_cmdQueue, &cmd, 0, 10) != osOK) {
                        CMD_FREE(cmd);  // ⚠️ 发送失败必须释放!
                        printf("[Button Adapter] Command queue full\n");
                    }
                }
            }
        }
        
        lastState = currentState;
        osDelay(10); // 10ms轮询间隔
    }
}
