/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : CommandDispatcher.h
  * @brief          : 命令分发器 - 路由命令到对应业务模块
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __COMMAND_DISPATCHER_H
#define __COMMAND_DISPATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "CommandDef.h"

/* ========== 命令分发器接口 ========== */
/**
  * @brief  初始化命令分发器
  * @param  cmdQueue: 输入命令队列句柄
  * @param  respQueue: 输出响应队列句柄
  * @retval None
  */
void CommandDispatcher_Init(osMessageQueueId_t cmdQueue, osMessageQueueId_t respQueue);

/**
  * @brief  命令分发器任务
  * @param  argument: 未使用
  * @retval None
  */
void CommandDispatcherTask(void *argument);

/**
  * @brief  注册命令处理回调函数
  * @param  command_id: 命令ID
  * @param  handler: 处理函数指针
  * @retval 0: 成功, -1: 失败
  */
int8_t CommandDispatcher_RegisterHandler(uint8_t command_id, 
                                         void (*handler)(Command_t *cmd, Response_t *resp));

#ifdef __cplusplus
}
#endif

#endif /* __COMMAND_DISPATCHER_H */
