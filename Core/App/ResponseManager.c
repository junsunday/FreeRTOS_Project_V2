/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ResponseManager.c
  * @brief          : 响应管理器实现 - 统一输出管理
  ******************************************************************************
  */
/* USER CODE END Header */

#include "ResponseManager.h"
#include "main.h"
#include "usart.h"
#include "usb_device.h"
#include "usbd_custom_hid_if.h"
#include <stdio.h>
#include <string.h>
// #include "UartInputAdapter.h"

/* ========== 私有变量 ========== */
static osMessageQueueId_t s_respQueue = NULL;

/* ========== 私有函数声明 ========== */
static int8_t SendToHID(Response_t *resp);
static int8_t SendToUART(Response_t *resp);
static void BuildUartFrame(Response_t *resp, uint8_t *frame, uint16_t *frameLen);

/* ========== 公共函数实现 ========== */

/**
  * @brief  初始化响应管理器
  */
void ResponseManager_Init(osMessageQueueId_t respQueue)
{
    s_respQueue = respQueue;
}

/**
  * @brief  响应管理器任务
  */
void ResponseManagerTask(void *argument)
{
    printf("[Response Manager] Task started\n");
    
    for (;;) {
        Response_t *resp = NULL;  // ⚠️ 使用指针
        
        // 从响应队列接收响应指针(阻塞等待)
        if (osMessageQueueGet(s_respQueue, &resp, NULL, osWaitForever) == osOK) {
            if (resp == NULL) {
                printf("[ResponseMgr] ❌ Received NULL response pointer\n");
                continue;
            }
            
            // 根据目标通道发送响应
            if (resp->target_channel == CHAN_HID || resp->target_channel == CHAN_BOTH) {
                if (SendToHID(resp) != 0) {
                    printf("[ResponseMgr] HID send failed\n");
                }
            }
            
            if (resp->target_channel == CHAN_UART || resp->target_channel == CHAN_BOTH) {
                // if (SendToUART(resp) != 0) {
                //     printf("[ResponseMgr] UART send failed\n");
                // }
            }
            
            // ✅ 响应发送完毕,释放内存
            RESP_FREE(resp);
        }
    }
}

/**
  * @brief  发送响应到指定通道
  */
int8_t ResponseManager_Send(Response_t *resp)
{
    if (resp == NULL || s_respQueue == NULL) {
        return -1;
    }
    
    if (osMessageQueuePut(s_respQueue, resp, 0, 10) != osOK) {
        return -1;
    }
    
    return 0;
}

/* ========== 私有函数实现 ========== */

/**
  * @brief  发送到HID通道
  */
static int8_t SendToHID(Response_t *resp)
{
    // 检查USB设备状态
    if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
        printf("[ResponseMgr] ❌ USB not configured (state=%d)\n", hUsbDeviceFS.dev_state);
        return -1;
    }
    
    // 使用静态缓冲区,避免异步发送时栈数据被破坏
    static uint8_t hidData[64];  // static: 生命周期贯穿整个程序运行
    
    // 准备发送数据(最多64字节)
    memset(hidData, 0, sizeof(hidData));  // 清零
    uint16_t copyLen = (resp->data_len > 62) ? 62 : resp->data_len;  // 预留2字节给command_id和status
    
    // ✅ 构建HID数据包: [命令ID][状态][数据...]
    hidData[0] = resp->command_id;
    hidData[1] = resp->status;
    if (copyLen > 0) {
        memcpy(&hidData[2], resp->data, copyLen);
    }
    
    // ✅ 调试日志: 打印即将发送的数据
    printf("[HID TX] cmd=0x%02X status=0x%02X len=%d data:", 
           hidData[0], hidData[1], copyLen + 2);
    for (uint16_t i = 0; i < copyLen + 2 && i < 16; i++) {
        printf(" %02X", hidData[i]);
    }
    if (copyLen + 2 > 16) printf(" ...");
    printf("\n");
    
    // ✅ 重试机制: 处理USBD_BUSY情况
    int8_t result;
    uint8_t retry = 3;
    while (retry-- > 0) {
        result = USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, hidData, 64);
        
        if (result == USBD_OK) {
            break;  // 发送成功
        } else if (result == USBD_BUSY) {
            printf("[ResponseMgr] ⚠️ HID endpoint busy, retry (%d left)\n", retry);
            osDelay(5);  // 等待5ms后重试
        } else {
            printf("[ResponseMgr] ❌ HID send failed: result=%d\n", result);
            return -1;
        }
    }
    
    if (result != USBD_OK) {
        printf("[ResponseMgr] ❌ HID send failed after retries\n");
        return -1;
    }
    return 0;
}

/**
  * @brief  发送到UART通道
  */
static int8_t SendToUART(Response_t *resp)
{
    uint8_t frame[128] = {0};
    uint16_t frameLen = 0;
    
    // 构建协议帧
    BuildUartFrame(resp, frame, &frameLen);
    
    // 发送
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart1, frame, frameLen, HAL_MAX_DELAY);
    return (status == HAL_OK) ? 0 : -1;
}

/**
  * @brief  构建UART协议帧
  * @note   帧格式: [帧头AA][长度高][长度低][标志BB][命令ID][状态][数据...][校验和]
  */
static void BuildUartFrame(Response_t *resp, uint8_t *frame, uint16_t *frameLen)
{
    uint16_t payloadLen = 2 + resp->data_len; // 命令ID + 状态 + 数据
    uint16_t totalLen = 5 + payloadLen; // 帧头 + 长度 + 标志 + 命令ID + 状态 + 数据 + 校验和
    
    // 帧头
    frame[0] = 0xAA;
    
    // 长度字段(大端序)
    frame[1] = (payloadLen >> 8) & 0xFF;
    frame[2] = payloadLen & 0xFF;
    
    // 标志
    frame[3] = 0xBB;
    
    // Payload: 命令ID + 状态 + 数据
    frame[4] = resp->command_id;
    frame[5] = resp->status;
    if (resp->data_len > 0) {
        memcpy(&frame[6], resp->data, resp->data_len);
    }
    
    // 计算校验和
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < totalLen - 1; i++) {
        checksum += frame[i];
    }
    frame[totalLen - 1] = checksum;
    
    *frameLen = totalLen;
}
