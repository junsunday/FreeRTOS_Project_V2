/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : CommandDispatcher.c
  * @brief          : 命令分发器实现
  ******************************************************************************
  */
/* USER CODE END Header */

#include "CommandDispatcher.h"
#include "LedController.h"
// #include "OledShowController.h"
#include "SystemMonitor.h"
#include "cmsis_os2.h"
#include "main.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ========== 命令处理函数类型定义 ========== */
typedef void (*CommandHandler_t)(Command_t *cmd, Response_t *resp);

/* ========== 私有变量 ========== */
static osMessageQueueId_t s_cmdQueue = NULL;
static osMessageQueueId_t s_respQueue = NULL;

// 命令处理表(最多支持16个命令)
#define MAX_HANDLERS 16
typedef struct {
    uint8_t command_id;
    CommandHandler_t handler;
} HandlerEntry_t;

static HandlerEntry_t s_handlerTable[MAX_HANDLERS] = {0};
static uint8_t s_handlerCount = 0;

/* ========== 私有函数声明 ========== */
static CommandHandler_t FindHandler(uint8_t command_id);
static void SendErrorResponse(uint8_t cmd_id, uint8_t status, uint8_t target_channel);

/* ========== 公共函数实现 ========== */
/**
  * @brief  HID透传测试命令处理器
  */
void HID_test(Command_t *cmd, Response_t *resp)
{
    if (cmd == NULL || resp == NULL) {
        return;
    }
    
    // ✅ 修复: 使用安全的打印方式,指定长度避免越界
    printf("[HID Test] Received %d bytes: ", cmd->payload_len);
    for (uint16_t i = 0; i < cmd->payload_len && i < CMD_PAYLOAD_MAX_LEN; i++) {
        printf("%02X ", cmd->payload[i]);
    }
    printf("\n");
    
    // 构建响应(回显数据)
    resp->data_len = cmd->payload_len;
    if (resp->data_len > 0) {
        memcpy(resp->data, cmd->payload, resp->data_len);
    }
}
extern MPU6050Frame_t s_mpu6050Data;
void MPU6050_HandleCommand(Command_t *cmd, Response_t *resp)
{
        if (cmd == NULL || resp == NULL) {
        return;
    }
    
    printf("status: %.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", 
                       s_mpu6050Data.accel[0], s_mpu6050Data.accel[1], s_mpu6050Data.accel[2],
                       s_mpu6050Data.gyro[0], s_mpu6050Data.gyro[1], s_mpu6050Data.gyro[2],
                       s_mpu6050Data.angle[0], s_mpu6050Data.angle[1], s_mpu6050Data.angle[2],
                       s_mpu6050Data.temperature);
    // 构建响应(回显数据)
    resp->data_len = cmd->payload_len;
    if (resp->data_len > 0) {
        memcpy(resp->data, cmd->payload, resp->data_len);
    }
}
/**
  * @brief  初始化命令分发器
  */
void CommandDispatcher_Init(osMessageQueueId_t cmdQueue, osMessageQueueId_t respQueue)
{
    s_cmdQueue = cmdQueue;
    s_respQueue = respQueue;
    s_handlerCount = 0;
    memset(s_handlerTable, 0, sizeof(s_handlerTable));
    
    // 注册内置命令处理器
    CommandDispatcher_RegisterHandler(CMD_LED_CONTROL, LedController_HandleCommand);
    CommandDispatcher_RegisterHandler(CMD_LED_BREATH_START, LedController_HandleCommand);
    CommandDispatcher_RegisterHandler(CMD_LED_BREATH_STOP, LedController_HandleCommand);
    CommandDispatcher_RegisterHandler(CMD_SYS_STATUS_QUERY, SystemMonitor_HandleCommand);
    // lk
    CommandDispatcher_RegisterHandler(CMD_DATA_PASSTHROUGH, HID_test);
    // CommandDispatcher_RegisterHandler(CMD_OLED_CTRL, OledShowCtrl_HandleCommand);
    CommandDispatcher_RegisterHandler(CMD_MPU6050_DATA, MPU6050_HandleCommand);
}

/**
  * @brief  注册命令处理回调
  */
int8_t CommandDispatcher_RegisterHandler(uint8_t command_id, CommandHandler_t handler)
{
    if (s_handlerCount >= MAX_HANDLERS || handler == NULL) {
        return -1;
    }
    
    // 检查是否已注册
    for (uint8_t i = 0; i < s_handlerCount; i++) {
        if (s_handlerTable[i].command_id == command_id) {
            s_handlerTable[i].handler = handler; // 更新已有处理器
            return 0;
        }
    }
    
    // 添加新处理器
    s_handlerTable[s_handlerCount].command_id = command_id;
    s_handlerTable[s_handlerCount].handler = handler;
    s_handlerCount++;
    
    return 0;
}
extern osMessageQueueId_t CommandQueueHandle;
/**
  * @brief  命令分发器任务
  */
void CommandDispatcherTask(void *argument)
{
    Response_t *resp;
    CommandHandler_t handler;
    
    printf("[Command Dispatcher] Task started, waiting for commands...\n");
    
    for (;;) {
        Command_t *cmd = NULL;  // ⚠️ 使用指针
        
        // 从命令队列接收命令指针(阻塞等待)
        if (osMessageQueueGet(s_cmdQueue, &cmd, NULL, osWaitForever) == osOK) {
            if (cmd == NULL) {
                printf("[Dispatcher] ❌ Received NULL command pointer\n");
                continue;
            }
            
            // printf("[Dispatcher] ✅ Received command: 0x%02X from src=%d\n", 
            //        cmd->command_id, cmd->source);
            
            // 查找对应的处理器
            handler = FindHandler(cmd->command_id);
            
            if (handler != NULL) {
                // ⚠️ 从堆分配响应对象
                resp = RESP_MALLOC();
                if (resp == NULL) {
                    printf("[Dispatcher] ❌ Response memory allocation failed\n");
                    CMD_FREE(cmd);  // ⚠️ 必须释放命令!
                    continue;
                }
                
                // 初始化响应结构
                memset(resp, 0, sizeof(Response_t));
                resp->command_id = cmd->command_id;
                resp->status = CMD_STATUS_SUCCESS;
                
                // 根据命令来源确定响应通道
                if (cmd->source == CMD_SRC_HID) {
                    resp->target_channel = CHAN_HID;
                } else if (cmd->source == CMD_SRC_UART) {
                    resp->target_channel = CHAN_UART;
                } else {
                    resp->target_channel = CHAN_BOTH;
                }
                
                // 调用处理器
                handler(cmd, resp);
                
                // ✅ 发送响应指针到队列(所有权转移给ResponseManager)
                if (osMessageQueuePut(s_respQueue, &resp, 0, 10) != osOK) {
                    RESP_FREE(resp);  // ⚠️ 发送失败必须释放!
                    printf("[Dispatcher] Response queue full\n");
                }
                // 注意: resp 成功发送后由 ResponseManager 释放
                
            } else {
                // 未找到处理器,发送错误响应
                printf("[Dispatcher] Unknown command ID: 0x%02X\n", cmd->command_id);
                SendErrorResponse(cmd->command_id, CMD_STATUS_INVALID, 
                                 (cmd->source == CMD_SRC_HID) ? CHAN_HID : CHAN_UART);
            }
            
            // ✅ 命令处理完毕,释放内存
            CMD_FREE(cmd);
        }
    }
}

/* ========== 私有函数实现 ========== */

/**
  * @brief  查找命令处理器
  */
static CommandHandler_t FindHandler(uint8_t command_id)
{
    for (uint8_t i = 0; i < s_handlerCount; i++) {
        if (s_handlerTable[i].command_id == command_id) {
            return s_handlerTable[i].handler;
        }
    }
    return NULL;
}

/**
  * @brief  发送错误响应
  */
static void SendErrorResponse(uint8_t cmd_id, uint8_t status, uint8_t target_channel)
{
    // ⚠️ 从堆分配响应对象
    Response_t *resp = RESP_MALLOC();
    if (resp == NULL) {
        printf("[Dispatcher] ❌ Error response memory allocation failed\n");
        return;
    }
    
    memset(resp, 0, sizeof(Response_t));
    resp->command_id = cmd_id;
    resp->status = status;
    resp->data_len = 0;
    resp->target_channel = target_channel;
    
    if (osMessageQueuePut(s_respQueue, &resp, 0, 10) != osOK) {
        RESP_FREE(resp);  // ⚠️ 发送失败必须释放!
        printf("[Dispatcher] Error response queue full\n");
    }
}
