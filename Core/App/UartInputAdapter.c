/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : UartInputAdapter.c
  * @brief          : UART输入适配器实现 - 包含协议解析
  ******************************************************************************
  */
/* USER CODE END Header */

#include "InputAdapters.h"
#include "CommandDef.h"
#include "main.h"
#include "usart.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* ========== 协议定义 ========== */
#define FRAME_HEADER          0xAA
#define FRAME_FLAG            0xBB
#define FRAME_HEADER_SIZE     1
#define FRAME_LENGTH_SIZE     2
#define FRAME_FLAG_SIZE       1
#define FRAME_CHECKSUM_SIZE   1
#define FRAME_OVERHEAD        (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE + FRAME_CHECKSUM_SIZE)
#define MAX_COMMAND_LENGTH    80

/* ========== 协议解析状态机 ========== */
typedef enum {
    PARSE_STATE_IDLE = 0,
    PARSE_STATE_RECEIVING
} ParseState_t;

typedef struct {
    uint8_t buffer[MAX_COMMAND_LENGTH];
    uint16_t count;
    uint16_t expectedLength;
    ParseState_t state;
} ProtocolParser_t;

/* ========== 私有变量 ========== */
static osMessageQueueId_t s_cmdQueue = NULL;
static ProtocolParser_t s_parser = {0};
static uint8_t s_rxByte = 0;

/* ========== 私有函数声明 ========== */
static uint8_t CalculateChecksum(const uint8_t *data, uint16_t length);
static uint8_t ValidateFrame(const uint8_t *data, uint16_t totalLen, uint16_t payloadLen);
static void ProcessCompleteFrame(void);
static void UartReceiveCallback(uint8_t data);

/* ========== 公共函数实现 ========== */

/**
  * @brief  初始化UART输入适配器
  */
void UartInputAdapter_Init(osMessageQueueId_t cmdQueue)
{
    s_cmdQueue = cmdQueue;
    memset(&s_parser, 0, sizeof(ProtocolParser_t));
    s_parser.state = PARSE_STATE_IDLE;
    
    // 启动串口接收中断
    StartReceive_IT(&huart1);
}

/**
  * @brief  UART输入适配器任务
  */
void UartInputAdapter_Task(void *argument)
{
    for (;;) {
        // 从消息队列读取串口数据(由中断回调发送)
        if (osMessageQueueGet(CommandQueueHandle, &s_rxByte, NULL, 10) == osOK) {
            UartReceiveCallback(s_rxByte);
        }
        
        osDelay(5);
    }
}

/* ========== 私有函数实现 ========== */

/**
  * @brief  计算校验和
  */
static uint8_t CalculateChecksum(const uint8_t *data, uint16_t length)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

/**
  * @brief  验证帧完整性
  */
static uint8_t ValidateFrame(const uint8_t *data, uint16_t totalLen, uint16_t payloadLen)
{
    if (totalLen < FRAME_OVERHEAD) {
        return 0;
    }
    
    // 提取长度字段
    uint16_t framePayloadLen = ((uint16_t)data[1] << 8) | data[2];
    
    // 验证长度匹配
    if (framePayloadLen != payloadLen) {
        return 0;
    }
    
    if (totalLen != (FRAME_OVERHEAD + payloadLen)) {
        return 0;
    }
    
    // 验证校验和
    uint8_t calcChecksum = CalculateChecksum(data, totalLen - 1);
    uint8_t recvChecksum = data[totalLen - 1];
    
    return (calcChecksum == recvChecksum) ? 1 : 0;
}

/**
  * @brief  处理完整帧
  */
static void ProcessCompleteFrame(void)
{
    Command_t cmd;
    uint16_t payloadLen = s_parser.expectedLength;
    uint16_t totalLen = FRAME_OVERHEAD + payloadLen;
    
    // 验证帧
    if (ValidateFrame(s_parser.buffer, totalLen, payloadLen) != 1) {
        printf("[UART Adapter] Frame validation failed\n");
        return;
    }
    
    // 构建标准命令
    cmd.source = CMD_SRC_UART;
    cmd.command_id = s_parser.buffer[FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE]; // payload第1字节为命令ID
    cmd.payload_len = payloadLen - 1; // 减去命令ID字节
    
    if (cmd.payload_len > 0 && cmd.payload_len <= CMD_PAYLOAD_MAX_LEN) {
        memcpy(cmd.payload, &s_parser.buffer[FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE + 1], 
               cmd.payload_len);
    }
    
    cmd.timestamp = osKernelGetTickCount();
    
    // 发送到命令队列
    if (osMessageQueuePut(s_cmdQueue, &cmd, 0, 10) != osOK) {
        printf("[UART Adapter] Command queue full\n");
    } else {
        // 调试回传: 将完整帧发回串口
        HAL_UART_Transmit(&huart1, s_parser.buffer, totalLen, HAL_MAX_DELAY);
    }
}

/**
  * @brief  串口接收回调(处理单个字节)
  */
static void UartReceiveCallback(uint8_t data)
{
    switch (s_parser.state) {
        case PARSE_STATE_IDLE:
            if (data == FRAME_HEADER) {
                s_parser.buffer[0] = data;
                s_parser.count = 1;
                s_parser.state = PARSE_STATE_RECEIVING;
            }
            break;
            
        case PARSE_STATE_RECEIVING:
            if (s_parser.count >= MAX_COMMAND_LENGTH) {
                // 缓冲区溢出,重置
                s_parser.count = 0;
                s_parser.state = PARSE_STATE_IDLE;
                
                // 如果当前字节是帧头,重新开始
                if (data == FRAME_HEADER) {
                    s_parser.buffer[0] = data;
                    s_parser.count = 1;
                    s_parser.state = PARSE_STATE_RECEIVING;
                }
                break;
            }
            
            s_parser.buffer[s_parser.count++] = data;
            
            // 检查标志位
            if (s_parser.count == (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE)) {
                if (data != FRAME_FLAG) {
                    s_parser.count = 0;
                    s_parser.state = PARSE_STATE_IDLE;
                    break;
                }
            }
            
            // 接收完长度字段
            if (s_parser.count == (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE)) {
                s_parser.expectedLength = ((uint16_t)s_parser.buffer[1] << 8) | s_parser.buffer[2];
                
                // 验证长度合理性
                if (s_parser.expectedLength > (MAX_COMMAND_LENGTH - FRAME_OVERHEAD)) {
                    s_parser.count = 0;
                    s_parser.state = PARSE_STATE_IDLE;
                    break;
                }
            }
            
            // 检查是否接收完整帧
            if (s_parser.count == (FRAME_OVERHEAD + s_parser.expectedLength)) {
                ProcessCompleteFrame();
                
                // 重置状态机
                s_parser.count = 0;
                s_parser.state = PARSE_STATE_IDLE;
            }
            break;
            
        default:
            s_parser.count = 0;
            s_parser.state = PARSE_STATE_IDLE;
            break;
    }
}
