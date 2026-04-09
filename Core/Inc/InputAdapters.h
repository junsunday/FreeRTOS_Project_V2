/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : InputAdapters.h
  * @brief          : 输入适配器层 - HID/UART/Button输入封装
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __INPUT_ADAPTERS_H
#define __INPUT_ADAPTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "CommandDef.h"

/* ========== HID输入适配器 ========== */
/**
  * @brief  初始化HID输入适配器
  * @param  cmdQueue: 命令队列句柄
  * @retval None
  */
void HidInputAdapter_Init(osMessageQueueId_t cmdQueue);

/**
  * @brief  HID输入适配器任务
  * @param  argument: 未使用
  * @retval None
  */
void HidAdapterTask(void *argument);

/**
  * @brief  处理HID接收数据(在中断或回调中调用)
  * @param  data: 接收到的数据
  * @param  len: 数据长度
  * @retval None
  */
void HidInputAdapter_ProcessData(uint8_t *data, uint16_t len);

/* ========== UART输入适配器 ========== */
/**
  * @brief  初始化UART输入适配器
  * @param  cmdQueue: 命令队列句柄
  * @retval None
  */
void UartInputAdapter_Init(osMessageQueueId_t cmdQueue);

/**
  * @brief  UART输入适配器任务
  * @param  argument: 未使用
  * @retval None
  */
void UartAdapterTask(void *argument);

/* ========== Button输入适配器 ========== */
/**
  * @brief  初始化Button输入适配器
  * @param  cmdQueue: 命令队列句柄
  * @retval None
  */
void ButtonInputAdapter_Init(osMessageQueueId_t cmdQueue);

/**
  * @brief  Button输入适配器任务
  * @param  argument: 未使用
  * @retval None
  */
void ButtonAdapterTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* __INPUT_ADAPTERS_H */
