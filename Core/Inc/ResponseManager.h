/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : ResponseManager.h
  * @brief          : 响应管理器 - 统一管理输出通道
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __RESPONSE_MANAGER_H
#define __RESPONSE_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "CommandDef.h"

/* ========== 响应管理器接口 ========== */
/**
  * @brief  初始化响应管理器
  * @param  respQueue: 响应队列句柄
  * @retval None
  */
void ResponseManager_Init(osMessageQueueId_t respQueue);

/**
  * @brief  响应管理器任务
  * @param  argument: 未使用
  * @retval None
  */
void ResponseManagerTask(void *argument);

/**
  * @brief  发送响应到指定通道
  * @param  resp: 响应结构指针
  * @retval 0: 成功, -1: 失败
  */
int8_t ResponseManager_Send(Response_t *resp);

#ifdef __cplusplus
}
#endif

#endif /* __RESPONSE_MANAGER_H */
