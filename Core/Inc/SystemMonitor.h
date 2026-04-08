/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : SystemMonitor.h
  * @brief          : 系统监控器 - 业务逻辑层
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __SYSTEM_MONITOR_H
#define __SYSTEM_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CommandDef.h"

/* ========== 系统监控器接口 ========== */
/**
  * @brief  初始化系统监控器
  * @retval None
  */
void SystemMonitor_Init(void);

/**
  * @brief  处理系统监控命令
  * @param  cmd: 命令结构指针
  * @param  resp: 响应结构指针
  * @retval None
  */
void SystemMonitor_HandleCommand(Command_t *cmd, Response_t *resp);

/**
  * @brief  获取系统状态信息
  * @param  statusData: 状态数据缓冲区
  * @param  maxLen: 缓冲区最大长度
  * @retval 实际写入的数据长度
  */
uint16_t SystemMonitor_GetStatus(uint8_t *statusData, uint16_t maxLen);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_MONITOR_H */
