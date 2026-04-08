#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "main.h"
#include "FreeRTOS.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// #include "stm32f1xx_hal_uart.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"

// ========== 环形缓冲区配置 ==========
#define RING_BUFFER_SIZE      64
#define LEDControl            0xFF
#define MAX_COMMAND_LENGTH    30    // 命令最大长度（字节）
#define FRAME_HEADER          0xAA  // 帧头
#define FRAME_FLAG            0xBB  // 帧标志
#define FRAME_HEADER_SIZE     1     // 帧头长度
#define FRAME_LENGTH_SIZE     2     // 长度字段长度（字节）
#define FRAME_FLAG_SIZE       1     // 标志字段长度（字节）
#define FRAME_CHECKSUM_SIZE   1     // 校验和长度（字节）
#define FRAME_OVERHEAD        (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE + FRAME_CHECKSUM_SIZE)

// ========== 协议解析状态机 ==========
typedef enum {
    PARSE_STATE_IDLE = 0,     // 空闲状态，等待帧头
    PARSE_STATE_RECEIVING,    // 接收数据中
    PARSE_STATE_COMPLETE      // 接收完成（内部使用）
} ParseState_t;

// ========== 全局变量 ==========
// 环形缓冲区结构体
typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];
    uint16_t head;  // 写入位置
    uint16_t tail;  // 读取位置
    uint16_t count; // 当前数据数量
} RingBuffer_t;

// 全局环形缓冲区实例
static RingBuffer_t g_ringBuffer = {0};

/**
 * @brief 初始化环形缓冲区
 * @param  None
 * @retval None
 */
static void RingBuffer_Init(void)
{
    memset(&g_ringBuffer, 0, sizeof(RingBuffer_t));
}

/**
 * @brief 向环形缓冲区写入数据
 * @param data: 要写入的数据
 * @retval pdTRUE: 成功，pdFALSE: 缓冲区满
 */
static BaseType_t RingBuffer_Put(uint8_t data)
{
    if (g_ringBuffer.count >= RING_BUFFER_SIZE) {
        return pdFALSE; // 缓冲区已满
    }
    
    g_ringBuffer.buffer[g_ringBuffer.head] = data;
    g_ringBuffer.head = (g_ringBuffer.head + 1) % RING_BUFFER_SIZE;
    g_ringBuffer.count++;
    
    return pdTRUE;
}

/**
 * @brief 从环形缓冲区读取数据
 * @param pData: 存储读取数据的指针
 * @retval pdTRUE: 成功，pdFALSE: 缓冲区空
 */
static BaseType_t RingBuffer_Get(uint8_t *pData)
{
    if (g_ringBuffer.count == 0 || pData == NULL) {
        return pdFALSE; // 缓冲区为空或无效指针
    }
    
    *pData = g_ringBuffer.buffer[g_ringBuffer.tail];
    g_ringBuffer.tail = (g_ringBuffer.tail + 1) % RING_BUFFER_SIZE;
    g_ringBuffer.count--;
    
    return pdTRUE;
}

/**
 * @brief 获取缓冲区中的数据数量
 * @retval 当前缓冲区中的数据字节数
 */
static uint16_t RingBuffer_GetCount(void)
{
    return g_ringBuffer.count;
}

/**
 * @brief 计算校验和
 * @param data: 数据指针
 * @param length: 数据长度
 * @retval 计算得到的校验和
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
 * @brief 验证接收到的命令帧
 * @param commandData: 命令数据缓冲区
 * @param totalLength: 总长度
 * @param expectedPayloadLength: 期望的 payload 长度
 * @retval pdTRUE: 验证通过，pdFALSE: 验证失败
 */
static BaseType_t ValidateCommandFrame(const uint8_t *commandData, 
                                       uint16_t totalLength,
                                       uint16_t expectedPayloadLength)
{
    // 验证总长度合理性
    if (totalLength < FRAME_OVERHEAD) {
        return pdFALSE;
    }
    
    // 提取长度字段 (大端序)
    uint16_t payloadLength = ((uint16_t)commandData[1] << 8) | commandData[2];
    
    // 验证长度字段与实际数据是否匹配
    if (payloadLength != expectedPayloadLength) {
        return pdFALSE;
    }
    
    // 验证总长度是否匹配
    if (totalLength != (FRAME_OVERHEAD + payloadLength)) {
        return pdFALSE;
    }
    
    // 验证校验和 (最后 1 字节为校验和)
    uint8_t calculatedChecksum = CalculateChecksum(commandData, totalLength - 1);
    uint8_t receivedChecksum = commandData[totalLength - 1];
    
    if (calculatedChecksum != receivedChecksum) {
        return pdFALSE;
    }
    
    return pdTRUE;
}



/**
 * @brief 处理有效的 LED 控制命令
 * @param commandData: 命令数据缓冲区
 * @param payloadLength: payload 长度
 * @retval None
 */
static void ProcessLEDCommand(const uint8_t *commandData, uint16_t payloadLength)
{
    if (payloadLength != 2) {
        return; // LED 命令需要 2 字节 payload
    }
    
    // 提取 LED 状态 (commandData[4] 是 payload 的第 2 字节)
    uint8_t ledState = commandData[FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE + payloadLength - 1];
    
    // 发送命令到 LED 任务队列
    if (osMessageQueuePut(LEDStateQueueHandle, &ledState, 0, osWaitForever) != osOK) {
        printf("Command send fail\n");
        return;
    }
    osEventFlagsSet(LedTriggerEventHandle, LED_TRIGGER_EVENT_SERIAL);
    
    // 调试回传：将完整命令发回串口
    uint16_t frameLength = FRAME_OVERHEAD + payloadLength;
    HAL_UART_Transmit(&huart1, commandData, frameLength, HAL_MAX_DELAY);
    
}

/**
 * @brief 协议解析器状态结构
 */
typedef struct {
    uint8_t commandData[MAX_COMMAND_LENGTH];  // 命令缓冲区
    uint16_t commandCount;                     // 当前已接收字节数
    uint16_t expectedLength;                   // 期望的 payload 长度
    ParseState_t state;                        // 当前解析状态
} ProtocolParser_t;

void StartvSerialHandlerTask(void *argument)
{
  /* USER CODE BEGIN StartvSerialHandlerTask */
  uint8_t rxData;
  BaseType_t status;
  uint8_t ringData;
  uint8_t dropByte;
  
  // 协议解析器状态
  static ProtocolParser_t parser = {
      .commandData = {0},
      .commandCount = 0,
      .expectedLength = 0,
      .state = PARSE_STATE_IDLE
  };
  
  // 初始化环形缓冲区
  RingBuffer_Init();
  StartReceive_IT(&huart1);
  HAL_UART_Transmit(&huart1, (uint8_t*)"ok", 2, HAL_MAX_DELAY);
  
  /* Infinite loop */
  for(;;)
  {
    // 从消息队列中读取串口数据（阻塞等待）
    status = osMessageQueueGet(CommandQueueHandle, &rxData, NULL, 0);
    
    if (status == osOK) {
      // 将接收到的数据存入环形缓冲区
      if (RingBuffer_Put(rxData) == pdFALSE) {
        // 缓冲区满，丢弃最旧的数据，保留新数据
        RingBuffer_Get(&dropByte);
        RingBuffer_Put(rxData);
      }
    }
    
    // 处理环形缓冲区中的数据
    while (RingBuffer_GetCount() > 0) {
      if (RingBuffer_Get(&ringData) != pdTRUE) {
        continue;
      }
      
      // 协议解析状态机
      switch (parser.state) {
        case PARSE_STATE_IDLE:
          // 等待帧头
          if (ringData == FRAME_HEADER) {
            parser.commandData[0] = ringData;
            parser.commandCount = 1;
            parser.state = PARSE_STATE_RECEIVING;
          }
          // 非帧头数据直接丢弃
          break;
          
        case PARSE_STATE_RECEIVING:
          // 检查缓冲区边界
          if (parser.commandCount >= MAX_COMMAND_LENGTH) {
            // 缓冲区溢出，重置状态机
            parser.commandCount = 0;
            parser.state = PARSE_STATE_IDLE;
            
            // 如果当前字节是帧头，重新开始
            if (ringData == FRAME_HEADER) {
              parser.commandData[0] = ringData;
              parser.commandCount = 1;
              parser.state = PARSE_STATE_RECEIVING;
            }
            break;
          }
          
          // 存储数据
          parser.commandData[parser.commandCount++] = ringData;
          
          if (parser.commandCount == (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE)) {
            if (ringData != FRAME_FLAG){
              // 标志字段不正确，丢弃此帧
              parser.commandCount = 0;
              parser.state = PARSE_STATE_IDLE;
              break;
            }
          }
          // 接收完长度字段 (帧头 + 2 字节长度)
          if (parser.commandCount == (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE)) {
            // 提取 payload 长度 (大端序)
            parser.expectedLength = ((uint16_t)parser.commandData[1] << 8) | parser.commandData[2];
            
            // 验证长度字段合理性
            if (parser.expectedLength > (MAX_COMMAND_LENGTH - FRAME_OVERHEAD)) {
              // 长度超出范围，丢弃此帧
              parser.commandCount = 0;
              parser.state = PARSE_STATE_IDLE;
              break;
            }
          }
          
          // 检查是否接收完整帧 (帧头 + 长度 + payload + 校验和)
          if (parser.commandCount == (FRAME_OVERHEAD + parser.expectedLength)) {
            // 验证帧完整性
            if (ValidateCommandFrame(parser.commandData, 
                                    parser.commandCount,
                                    parser.expectedLength) == pdTRUE) {
              // 验证通过，处理特定命令
              if (parser.expectedLength == 2 && 
                  parser.commandData[FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE + parser.expectedLength - 2] == LEDControl) {
                ProcessLEDCommand(parser.commandData, parser.expectedLength);
              }
            }
            // 无论验证成功与否，都重置状态机等待下一帧
            parser.commandCount = 0;
            parser.state = PARSE_STATE_IDLE;
          }
          break;
          
        default:
          // 未知状态，重置
          parser.commandCount = 0;
          parser.state = PARSE_STATE_IDLE;
          break;
      }
    }
    osDelay(10);
  }
  /* USER CODE END StartvSerialHandlerTask */
}
