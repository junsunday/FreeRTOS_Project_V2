#include "cmsis_os.h"
#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "stm32f103xe.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>
#include <stdio.h>
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_ll_tim.h"
#include "tim.h"
#include "timers.h"
#include "tim.h"
static uint8_t timerOn = 0;
void BreathLedCallback(void *argument)
{
  /* USER CODE BEGIN BreathLedCallback */
    static uint16_t brightness = 0;
    static int8_t direction = 1; // 1: 增加亮度, -1: 减少亮度

    // 更新PWM占空比
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, brightness);

    // 调整亮度
    brightness += direction * 20; // 每次调整10的步长

    // 反转方向
    if (brightness >= 1000) {
        brightness = 1000;
        direction = -1;
    } else if (brightness <= 0) {
        brightness = 0;
        direction = 1;
    }
  /* USER CODE END BreathLedCallback */
}
void BrtLedStaCallback(void *argument)
{
  /* USER CODE BEGIN BrtLedStaCallback */
    osTimerStop(BreathLedTimerHandle);
    timerOn = 0;
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

    // 关闭LED，重新配置GPIO管脚的方法,
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP; // 如果外部悬空，建议加上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
  /* USER CODE END BrtLedStaCallback */
}
void StartLedTask(void *argument)
{
    uint32_t eventFlag;
    uint8_t ledState = 0x00; // 默认LED状态为OFF
    /* Infinite loop */
    for(;;)
    {
        // 事件组方式
        // 等待LED触发事件
        eventFlag = osEventFlagsWait(LedTriggerEventHandle, LED_TRIGGER_EVENT_SERIAL | LED_TRIGGER_EVENT_BUTTON | LED_TRIGGER_EVENT_WKUP, osFlagsWaitAny, osWaitForever);
        if (eventFlag & LED_TRIGGER_EVENT_SERIAL) {
            // 切换LED状态，串口事件
            osMessageQueueGet(LEDStateQueueHandle, &ledState, 0, 0);
            // 更新LED状态
            HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, (ledState & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, (ledState & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, (ledState & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        }
        if (eventFlag & LED_TRIGGER_EVENT_BUTTON) {
            // 全部点亮,外部中断事件
            HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, GPIO_PIN_RESET);
            // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        }
        if (eventFlag & LED_TRIGGER_EVENT_WKUP) {
          if (!timerOn) {
            timerOn = 1;
            // 启动呼吸灯，唤醒事件
            osTimerStart(BreathLedTimerHandle, 20); // 500ms周期
            HAL_TIM_MspPostInit(&htim3); 
            HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
            // 设置按键触发， 5秒后关闭定时器。
            osTimerStart(BrtLedStaTimerHandle, 10000); 
          } else {
            // 已经在呼吸了，再次触发则重置定时器，保持呼吸状态
          xTimerReset(BrtLedStaTimerHandle, 0); // 重置呼吸灯定时器，确保从头开始
        }
      }
        
    }
}


        // // 外部中断信号量等待（非阻塞等待）
        // if (osSemaphoreAcquire(BinarySemButtonHandle, 0) == osOK)
        // {
        //     // 全部点亮
        //     HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, GPIO_PIN_RESET);
        //     HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, GPIO_PIN_RESET);
        //     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
        // }
        // // 从消息队列中获取LED状态（非阻塞等待）
        // while(osMessageQueueGet(LEDStateQueueHandle, &ledState, 0, 0) == osOK) {
        //     // 每个位控制一个LED，0=亮，1=灭
        //     HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin, (ledState & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        //     HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin, (ledState & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        //     HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, (ledState & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        // }
        // osDelay(10);