/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : LedController.h
  * @brief          : LED控制器 - 业务逻辑层
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __LED_CONTROLLER_H
#define __LED_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "CommandDef.h"

/* ========== LED控制器接口 ========== */
/**
  * @brief  初始化LED控制器
  * @retval None
  */
void LedController_Init(void);

/**
  * @brief  处理LED控制命令
  * @param  cmd: 命令结构指针
  * @param  resp: 响应结构指针
  * @retval None
  */
void LedController_HandleCommand(Command_t *cmd, Response_t *resp);

/**
  * @brief  启动呼吸灯效果
  * @retval None
  */
void LedController_StartBreath(void);

/**
  * @brief  停止呼吸灯效果
  * @retval None
  */
void LedController_StopBreath(void);

/**
  * @brief  设置LED状态
  * @param  ledState: LED状态字节 (bit0=RED, bit1=GREEN)
  * @retval None
  */
void LedController_SetState(uint8_t ledState);

#ifdef __cplusplus
}
#endif

#endif /* __LED_CONTROLLER_H */
