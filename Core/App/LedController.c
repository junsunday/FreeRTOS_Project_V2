/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : LedController.c
  * @brief          : LED控制器实现 - 业务逻辑层
  ******************************************************************************
  */
/* USER CODE END Header */

#include "LedController.h"
#include "main.h"
#include "tim.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>
/* ========== 私有变量 ========== */
extern osTimerId_t BreathLedTimerHandle;
extern osTimerId_t BrtLedStaTimerHandle;
extern void BreathLedCallback(void *argument);
extern void BrtLedStaCallback(void *argument);

static uint8_t s_timerOn = 0;       // 0:未启动,1:启动
static uint16_t s_brightness = 0;   // 呼吸灯亮度(0-1000)
static int8_t s_direction = 1;      // 呼吸灯亮度变化方向(1:增加, -1:减少)

/* ========== 私有函数声明 ========== */
static void UpdateLEDState(uint8_t ledState);

/* ========== 公共函数实现 ========== */

/**
  * @brief  初始化LED控制器
  */
void LedController_Init(void)
{
    s_timerOn = 0;
    s_brightness = 0;
    s_direction = 1;
}

/**
  * @brief  处理LED控制命令
  */
void LedController_HandleCommand(Command_t *cmd, Response_t *resp)
{
    if (cmd == NULL || resp == NULL) {
        return;
    }
    
    switch (cmd->command_id) {
        case CMD_LED_CONTROL:
            // LED状态控制
            if (cmd->payload_len >= 1) {
                uint8_t ledState = cmd->payload[0];
                UpdateLEDState(ledState);
                
                // 构建响应
                resp->data[0] = ledState;
                resp->data_len = 1;
            } else {
                resp->status = CMD_STATUS_INVALID;
            }
            break;
            
        case CMD_LED_BREATH_START:
            // 启动呼吸灯
            LedController_StartBreath();
            resp->data[0] = 0x01; // 已启动
            resp->data_len = 1;
            break;
            
        case CMD_LED_BREATH_STOP:
            // 停止呼吸灯
            LedController_StopBreath();
            resp->data[0] = 0x00; // 已停止
            resp->data_len = 1;
            break;
            
        default:
            resp->status = CMD_STATUS_INVALID;
            break;
    }
}

/**
  * @brief  启动呼吸灯效果
  */
void LedController_StartBreath(void)
{
    if (!s_timerOn) {
        s_timerOn = 1;
        
        // 初始化TIM3 PWM
        HAL_TIM_MspPostInit(&htim3);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
        
        // 启动呼吸灯定时器
        osTimerStart(BreathLedTimerHandle, 20);
        
        // 启动自动关闭定时器(10秒)
        osTimerStart(BrtLedStaTimerHandle, 10000);
        
        printf("[LED Controller] Breath mode started\n");
    } else {
        // 已经在运行,重置定时器
        osTimerStart(BrtLedStaTimerHandle, 10000);
        printf("[LED Controller] Breath timer reset\n");
    }
}

/**
  * @brief  停止呼吸灯效果
  */
void LedController_StopBreath(void)
{
    if (s_timerOn) {
        s_timerOn = 0;
        
        // 停止定时器
        osTimerStop(BreathLedTimerHandle);
        osTimerStop(BrtLedStaTimerHandle);
        
        // 停止PWM
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
        
        // 恢复GPIO为推挽输出并关闭LED
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
        
        printf("[LED Controller] Breath mode stopped\n");
    }
}

/**
  * @brief  设置LED状态
  */
void LedController_SetState(uint8_t ledState)
{
    UpdateLEDState(ledState);
}

/* ========== 私有函数实现 ========== */

/**
  * @brief  更新LED硬件状态
  */
static void UpdateLEDState(uint8_t ledState)
{
    HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, 
                     (ledState & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, 
                     (ledState & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief  呼吸灯回调函数(由定时器调用)
  * @note   此函数需要注册到BreathLedTimerHandle
  */
void BreathLedCallback(void *argument)
{
    // 更新PWM占空比
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, s_brightness);
    
    // 调整亮度
    s_brightness += s_direction * 20;
    
    // 反转方向
    if (s_brightness >= 1000) {
        s_brightness = 1000;
        s_direction = -1;
    } else if (s_brightness <= 0) {
        s_brightness = 0;
        s_direction = 1;
    }
}

/**
  * @brief  呼吸灯状态回调(由定时器调用)
  * @note   此函数需要注册到BrtLedStaTimerHandle
  */
void BrtLedStaCallback(void *argument)
{
    LedController_StopBreath();
}
