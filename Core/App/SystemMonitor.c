/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : SystemMonitor.c
  * @brief          : 系统监控器实现 - 业务逻辑层
  ******************************************************************************
  */
/* USER CODE END Header */
#include "SystemMonitor.h"
#include "main.h"
#include "cmsis_os2.h"

#include <stdio.h>
#include <string.h>
#include "usbd_custom_hid_if.h"
#include "usb_device.h"

/* ========== 私有变量 ========== */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* ========== 公共函数实现 ========== */

/**
  * @brief  初始化系统监控器
  */
void SystemMonitor_Init(void)
{
    // 初始化USB设备(仅调用一次)
    MX_USB_DEVICE_Init();
    
    printf("[System Monitor] Initialized\n");
}

/**
  * @brief  处理系统监控命令
  */
void SystemMonitor_HandleCommand(Command_t *cmd, Response_t *resp)
{
    if (cmd == NULL || resp == NULL) {
        return;
    }
    
    switch (cmd->command_id) {
        case CMD_SYS_STATUS_QUERY:
            // 查询系统状态
            resp->data_len = SystemMonitor_GetStatus(resp->data, CMD_PAYLOAD_MAX_LEN);
            break;
            
        case CMD_SYS_RESET:
            // 系统复位命令(预留)
            resp->status = CMD_STATUS_SUCCESS;
            resp->data[0] = 0x01;
            resp->data_len = 1;
            // TODO: 实现软复位逻辑
            break;
            
        default:
            resp->status = CMD_STATUS_INVALID;
            break;
    }
}

/**
  * @brief  获取系统状态信息
  */
uint16_t SystemMonitor_GetStatus(uint8_t *statusData, uint16_t maxLen)
{
    if (statusData == NULL || maxLen < 10) {
        return 0;
    }
    
    uint16_t offset = 0;
    
    // USB设备状态
    statusData[offset++] = hUsbDeviceFS.dev_state;
    
    // FreeRTOS堆栈使用情况(示例)
    // statusData[offset++] = (uint8_t)(uxTaskGetStackHighWaterMark(NULL) & 0xFF);
    
    // 系统运行时间(秒)
    uint32_t uptime = osKernelGetTickCount() / 1000;
    statusData[offset++] = (uptime >> 24) & 0xFF;
    statusData[offset++] = (uptime >> 16) & 0xFF;
    statusData[offset++] = (uptime >> 8) & 0xFF;
    statusData[offset++] = uptime & 0xFF;
    
    // 填充剩余字节为0
    while (offset < maxLen) {
        statusData[offset++] = 0;
    }
    
    return offset;
}
