/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : HidInputAdapter.c
  * @brief          : HID输入适配器实现
  ******************************************************************************
  */
/* USER CODE END Header */

#include "InputAdapters.h"
#include "CommandDef.h"
#include "main.h"
#include "usbd_custom_hid_if.h"
#include "usb_device.h"
#include <string.h>
#include <stdio.h>

/* ========== 私有变量 ========== */
static osMessageQueueId_t s_cmdQueue = NULL;
static uint8_t s_hidBuffer[64] = {0};
static volatile uint8_t s_dataReady = 0;

/* ========== 私有函数声明 ========== */
static void HidDataCallback(uint8_t *data, uint16_t len);

/* ========== 公共函数实现 ========== */

/**
  * @brief  初始化HID输入适配器
  */
void HidInputAdapter_Init(osMessageQueueId_t cmdQueue)
{
    s_cmdQueue = cmdQueue;
    memset(s_hidBuffer, 0, sizeof(s_hidBuffer));
    s_dataReady = 0;
}

/**
  * @brief  处理HID接收数据(由CUSTOM_HID_OutEvent_FS调用)
  */
void HidInputAdapter_ProcessData(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0 || len > 64) {
        return;
    }
    
    // 复制数据到缓冲区
    memcpy(s_hidBuffer, data, len);
    
    // 标记数据就绪
    s_dataReady = 1;
}

/**
  * @brief  HID输入适配器任务
  */
void HidInputAdapter_Task(void *argument)
{
    Command_t cmd;
    
    for (;;) {
        // 检查是否有新数据
        if (s_dataReady) {
            // 构建标准命令结构
            cmd.source = CMD_SRC_HID;
            cmd.command_id = CMD_DATA_PASSTHROUGH; // HID默认为透传模式
            memcpy(cmd.payload, s_hidBuffer, 64);
            cmd.payload_len = 64;
            cmd.timestamp = osKernelGetTickCount();
            
            // 发送到命令队列
            if (osMessageQueuePut(s_cmdQueue, &cmd, 0, 10) != osOK) {
                printf("[HID Adapter] Command queue full\n");
            }
            
            // 清除标志
            s_dataReady = 0;
            memset(s_hidBuffer, 0, sizeof(s_hidBuffer));
        }
        
        osDelay(5); // 5ms轮询间隔
    }
}
