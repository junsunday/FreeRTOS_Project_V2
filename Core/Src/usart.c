/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <stdio.h>
#include "cmsis_os2.h"
#include <string.h>

// UART2 DMA 接收缓冲区(使用 static 确保生命周期)
static uint8_t s_uart2_dma_buf[UART2_DMA_BUF_SIZE];
static volatile uint16_t s_uart2_rx_count = 0;  // 已接收数据计数
// 处理缓冲区:扩大以容纳更多数据(至少 2 倍 DMA 缓冲区)
static uint8_t s_uart2_process_buf[UART2_DMA_BUF_SIZE * 3];

/**
  * @brief  UART2 DMA 接收初始化
  * @note   启用空闲中断,配合 DMA 实现高效接收
  */
void UART2_DMA_Receive_Init(void)
{
    // // 清除之前的配置
    // HAL_UART_Abort(&huart2);
    
    // 启动 DMA 循环接收
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, s_uart2_dma_buf, UART2_DMA_BUF_SIZE);
    
    // 使能空闲中断(IDLE Interrupt)
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
}

/**
  * @brief  启动 UART2 DMA 接收(重新初始化后调用)
  */
void UART2_Start_DMA_Receive(void)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, s_uart2_dma_buf, UART2_DMA_BUF_SIZE);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
}

/**
  * @brief  获取 DMA 接收到的数据
  * @param  length: 输出参数,返回实际接收的字节数
  * @retval 指向接收数据缓冲区的指针
  */
uint8_t* UART2_GetReceivedData(uint16_t *length)
{
    if (length != NULL) {
        *length = s_uart2_rx_count;
    }
    return s_uart2_process_buf;
}

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Channel6;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */
 
  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

// 重定向 printf 到串口（适用于 ARM Compiler 5/6 等）
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
  return ch;
}

// 确保 __io_putchar 也被定义(适用于 GCC 等编译器)
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 100);
  return ch;
}

uint8_t Rx_Data;

void StartReceive_IT(UART_HandleTypeDef *huartx)
{
  // UART1 仍使用中断方式
  if (huartx->Instance == USART1) {
      HAL_UART_Receive_IT(huartx, (uint8_t *)&Rx_Data, 1);
  }
  // UART2 使用 DMA 方式,不在这里启动
}

/**
  * @brief  UART 空闲中断回调(DMA + IDLE 模式)
  * @note   当 UART 总线空闲时触发,表示一帧数据传输完成
  */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2) {
        // DMA 接收完成或检测到空闲
        s_uart2_rx_count = Size;
        
        // 调试:打印接收到的数据长度和原始数据(前几次)
        static uint8_t debug_init = 0;
        if (debug_init < 5) {
            printf("[DMA] Received %d bytes: ", Size);
            for (uint16_t i = 0; i < Size && i < 20; i++) {
                printf("%02X ", s_uart2_dma_buf[i]);
            }
            printf("\n");
            debug_init++;
        }
        
        // 复制数据到处理缓冲区(避免 DMA 缓冲区被覆盖)
        if (Size > 0 && Size <= sizeof(s_uart2_process_buf)) {
            memcpy(s_uart2_process_buf, s_uart2_dma_buf, Size);
            
            // 通知任务处理数据
            osEventFlagsSet(UartEventHandle, UART2_REC_FLAG);
        } else if (Size > sizeof(s_uart2_process_buf)) {
            printf("[DMA] ⚠️ Data overflow: %d > %u\n", Size, sizeof(s_uart2_process_buf));
        }
        
        // 重新启动 DMA 接收(循环模式)
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, s_uart2_dma_buf, UART2_DMA_BUF_SIZE);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    // osSemaphoreRelease(BinarySemUartHandle); // 通知uart任务有数据
    osEventFlagsSet(UartEventHandle, UART1_REC_FLAG);
    HAL_UART_Receive_IT(&huart1, &Rx_Data, 1);
  }
  // UART2 不再使用此回调,改用 HAL_UARTEx_RxEventCallback
}

/* USER CODE END 1 */

