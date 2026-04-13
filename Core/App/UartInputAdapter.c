/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : UartInputAdapter.c
  * @brief          : UART输入适配器实现 - 包含协议解析
  ******************************************************************************
  */
/* USER CODE END Header */

#include "InputAdapters.h"
#include "CommandDef.h"
#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "main.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
/* =========== MPU6050 =========== */
#define MPU6050_FRAME_HEADER 0x55
#define MPU6050_ACCEL_DATA   0x51
#define MPU6050_GYRO_DATA    0x52
#define MPU6050_ANGLE_DATA   0x53
#define MPU6050_FRAME_SIZE   11  // Header(1) + Type(1) + Data(8) + Checksum(1)
/* ========== 协议定义 ========== */
#define FRAME_HEADER          0xAA
#define FRAME_FLAG            0xBB
#define FRAME_HEADER_SIZE     1
#define FRAME_LENGTH_SIZE     2
#define FRAME_FLAG_SIZE       1
#define FRAME_CHECKSUM_SIZE   1
#define FRAME_OVERHEAD        (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE + FRAME_CHECKSUM_SIZE)
#define MAX_COMMAND_LENGTH    80

/* ========== 协议解析状态机 ========== */
typedef enum {
    PARSE_STATE_IDLE = 0,
    PARSE_STATE_RECEIVING
} ParseState_t;

typedef struct {
    uint8_t buffer[MAX_COMMAND_LENGTH];
    uint16_t count;
    uint16_t expectedLength;
    ParseState_t state;
} ProtocolParser_t;

// typedef struct {
//     float accel[3];
//     float gyro[3];
//     float angle[3];
//     float temperature;
// } MPU6050Frame_t;

/* ========== 私有变量 ========== */
static osMessageQueueId_t s_cmdQueue = NULL;
static ProtocolParser_t s_parser = {0};
MPU6050Frame_t s_mpu6050Data = {0};
/* ========== 私有函数声明 ========== */
static uint8_t CalculateChecksum(const uint8_t *data, uint16_t length);
static uint8_t ValidateFrame(const uint8_t *data, uint16_t totalLen, uint16_t payloadLen);
static void ProcessCompleteFrame(void);
static void UartReceiveCallback(uint8_t data);
static void mpu6050DecodeFrame(uint8_t *data);
extern uint8_t* UART2_GetReceivedData(uint16_t *len);
/* ========== 公共函数实现 ========== */

/**
  * @brief  初始化UART输入适配器
  */
void UartInputAdapter_Init(osMessageQueueId_t cmdQueue)
{
    s_cmdQueue = cmdQueue;
    memset(&s_parser, 0, sizeof(ProtocolParser_t));
    s_parser.state = PARSE_STATE_IDLE;
    
    // 启动串口接收
    StartReceive_IT(&huart1);  // UART1 仍使用中断方式
    UART2_DMA_Receive_Init();   // UART2 使用 DMA + 空闲中断
}

/**
  * @brief  UART输入适配器任务
  */
void UartAdapterTask(void *argument)
{
    printf("[UART Adapter] Task started\n");
    uint32_t flag = 0;
    for (;;) {
        // 从消息队列读取串口数据(由中断回调发送)
        // if (osSemaphoreAcquire(BinarySemUartHandle, osWaitForever) == osOK) {
        //     UartReceiveCallback(Rx_Data); // 处理接收到的字节
        // }
        flag = osEventFlagsWait(UartEventHandle, UART1_REC_FLAG | UART2_REC_FLAG, osFlagsWaitAny, osWaitForever);
        if (flag) {
            if (flag & UART1_REC_FLAG) {
                osEventFlagsClear(UartEventHandle, UART1_REC_FLAG);
                UartReceiveCallback(Rx_Data); // 处理接收到的字节
            }
            if (flag & UART2_REC_FLAG) {
                osEventFlagsClear(UartEventHandle, UART2_REC_FLAG);
                
                // 获取 DMA 接收到的数据
                uint16_t rx_len = 0;
                uint8_t *rx_data = UART2_GetReceivedData(&rx_len);
                
                if (rx_len == 0 || rx_data == NULL) {
                    continue;  // 无有效数据
                }
                
                // 调试:打印接收到的总长度(每20次打印一次,避免刷屏)
                static uint32_t rx_debug_count = 0;
                if (rx_debug_count++ % 20 == 0) {
                    printf("[UART2] Received %d bytes\n", rx_len);
                }
                
                // 解析 MPU6050 数据(搜索帧头同步 + 容错处理)
                uint16_t offset = 0;
                uint8_t valid_packets = 0;
                while (offset < rx_len) {
                    // 搜索帧头 0x55
                    if (rx_data[offset] != MPU6050_FRAME_HEADER) {
                        offset++;  // 跳过非帧头字节
                        continue;
                    }
                    
                    // 检查是否有足够的数据(11字节)
                    if (offset + MPU6050_FRAME_SIZE > rx_len) {
                        // 剩余数据不足一包,保留到下次处理(可选优化)
                        break;
                    }
                    
                    // 验证第二字节(应该是 0x51/0x52/0x53)
                    uint8_t frame_type = rx_data[offset + 1];
                    if (frame_type != MPU6050_ACCEL_DATA && 
                        frame_type != MPU6050_GYRO_DATA && 
                        frame_type != MPU6050_ANGLE_DATA) {
                        // 不是有效的 MPU6050 帧,跳过
                        offset++;
                        continue;
                    }
                    
                    // 尝试解析这一包
                    mpu6050DecodeFrame(&rx_data[offset]);
                    offset += MPU6050_FRAME_SIZE;
                    valid_packets++;
                }
                
                // // 统计信息(每50次打印一次)
                // static uint32_t stats_count = 0;
                // if (stats_count++ % 50 == 0) {
                //     printf("[UART2] Valid: %d, Errors in this batch\n", valid_packets);
                // }
                
                // 如果没有解析到任何有效包,且数据量较大,打印警告
                if (valid_packets == 0 && rx_len > 5) {
                    printf("[UART2] ⚠️ No valid packets in %d bytes\n", rx_len);
                }
                
                // ⚠️ 从堆分配命令对象
                Command_t *cmd = pvPortMalloc(sizeof(Command_t));
                if (cmd == NULL) {
                    printf("[UART Adapter] ❌ Memory allocation failed\n");
                    continue;  // 使用 continue 而非 return
                }
                
                // 构建标准命令
                cmd->source = CMD_SRC_UART;
                cmd->command_id = CMD_MPU6050_DATA;
                cmd->payload_len = sizeof(s_mpu6050Data);
                
                if (cmd->payload_len > 0 && cmd->payload_len <= CMD_PAYLOAD_MAX_LEN) {
                    memcpy(cmd->payload, &s_mpu6050Data, cmd->payload_len);
                }
                
                cmd->timestamp = osKernelGetTickCount();
                
                // ✅ 发送指针到队列(所有权转移)
                if (osMessageQueuePut(s_cmdQueue, &cmd, 0, 100) != osOK) {
                    vPortFree(cmd);  // ⚠️ 发送失败必须释放!
                    printf("[UART Adapter] Command queue full (timeout after 100ms)\n");
                }
            }
        }
    }
}

/* ========== 私有函数实现 ========== */
static void mpu6050DecodeFrame(uint8_t *dat)
{
    // 首先验证帧头和第二字节
    if (dat[0] != MPU6050_FRAME_HEADER) {
        return;  // 帧头错误
    }
    
    uint8_t frame_type = dat[1];
    if (frame_type != MPU6050_ACCEL_DATA && 
        frame_type != MPU6050_GYRO_DATA && 
        frame_type != MPU6050_ANGLE_DATA) {
        return;  // 无效的帧类型
    } 
    uint8_t checkSum_sum = 0x00;   // 累加校验
    for (uint8_t i = 0; i < 10; i++) {
        checkSum_sum += dat[i];
    }
    
    // MPU6050 JY901 模块通常使用累加校验(低8位)
    uint8_t checkSum = checkSum_sum & 0xFF;
    if (dat[10] != checkSum) return;
    if (dat[0] == MPU6050_FRAME_HEADER) {
        switch (dat[1]) {
            case MPU6050_ACCEL_DATA:
                // 处理加速度数据
                s_mpu6050Data.accel[0] = ((short)(dat[2] | (dat[3] << 8)))/32768.0f*16;
                s_mpu6050Data.accel[1] = ((short)(dat[4] | (dat[5] << 8)))/32768.0f*16;
                s_mpu6050Data.accel[2] = ((short)(dat[6] | (dat[7] << 8)))/32768.0f*16;
                s_mpu6050Data.temperature = ((short)(dat[8] | (dat[9] << 8)))/340.0f+36.25;
                break;
            case MPU6050_GYRO_DATA:
                // 处理角速度数据
                s_mpu6050Data.gyro[0] = ((short)(dat[2] | (dat[3] << 8)))/32768.0f*2000;
                s_mpu6050Data.gyro[1] = ((short)(dat[4] | (dat[5] << 8)))/32768.0f*2000;
                s_mpu6050Data.gyro[2] = ((short)(dat[6] | (dat[7] << 8)))/32768.0f*2000;
                s_mpu6050Data.temperature = ((short)(dat[8] | (dat[9] << 8)))/340.0f+36.25;
                break;
            case MPU6050_ANGLE_DATA:
                // 处理角度数据
                s_mpu6050Data.angle[0] = ((short)(dat[2] | (dat[3] << 8)))/32768.0f*180;
                s_mpu6050Data.angle[1] = ((short)(dat[4] | (dat[5] << 8)))/32768.0f*180;
                s_mpu6050Data.angle[2] = ((short)(dat[6] | (dat[7] << 8)))/32768.0f*180;
                s_mpu6050Data.temperature = ((short)(dat[8] | (dat[9] << 8)))/340.0f+36.25;
                break;
        }   
    }
}

/**
  * @brief  计算校验和
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
  * @brief  验证帧完整性
  */
static uint8_t ValidateFrame(const uint8_t *data, uint16_t totalLen, uint16_t payloadLen)
{
    if (totalLen < FRAME_OVERHEAD) {
        return 0;
    }
    
    // 提取长度字段
    uint16_t framePayloadLen = ((uint16_t)data[1] << 8) | data[2];
    
    // 验证长度匹配
    if (framePayloadLen != payloadLen) {
        return 0;
    }
    
    if (totalLen != (FRAME_OVERHEAD + payloadLen)) {
        return 0;
    }
    
    // 验证校验和
    uint8_t calcChecksum = CalculateChecksum(data, totalLen - 1);
    uint8_t recvChecksum = data[totalLen - 1];
    
    return (calcChecksum == recvChecksum) ? 1 : 0;
}

/**
  * @brief  处理完整帧
  */
static void ProcessCompleteFrame(void)
{
    uint16_t payloadLen = s_parser.expectedLength;
    uint16_t totalLen = FRAME_OVERHEAD + payloadLen;
    
    // 验证帧
    if (ValidateFrame(s_parser.buffer, totalLen, payloadLen) != 1) {
        printf("[UART Adapter] Frame validation failed\n");
        return;
    }
    
    // ⚠️ 从堆分配命令对象
    Command_t *cmd = pvPortMalloc(sizeof(Command_t));
    if (cmd == NULL) {
        printf("[UART Adapter] ❌ Memory allocation failed\n");
        return;
    }
    
    // 构建标准命令
    cmd->source = CMD_SRC_UART;
    cmd->command_id = s_parser.buffer[FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE];
    cmd->payload_len = payloadLen - 1; // 减去命令ID字节
    
    if (cmd->payload_len > 0 && cmd->payload_len <= CMD_PAYLOAD_MAX_LEN) {
        memcpy(cmd->payload, &s_parser.buffer[FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE + 1], 
               cmd->payload_len);
    }
    
    cmd->timestamp = osKernelGetTickCount();
    
    // ✅ 发送指针到队列(所有权转移)
    // 注意: 这要求 s_cmdQueue 创建时的 item_size 为 sizeof(Command_t*)
    if (osMessageQueuePut(s_cmdQueue, &cmd, 0, 100) != osOK) {
        vPortFree(cmd);  // ⚠️ 发送失败必须释放!
        printf("[UART Adapter] Command queue full (timeout after 100ms)\n");
    } else {
        // 调试回传(可选)
        // HAL_UART_Transmit(&huart1, s_parser.buffer, totalLen, HAL_MAX_DELAY);
    }
}

/**
  * @brief  串口接收回调(处理单个字节)
  */
static void UartReceiveCallback(uint8_t data)
{
    switch (s_parser.state) {
        case PARSE_STATE_IDLE:
            if (data == FRAME_HEADER) {
                s_parser.buffer[0] = data;
                s_parser.count = 1;
                s_parser.state = PARSE_STATE_RECEIVING;
            }
            break;
            
        case PARSE_STATE_RECEIVING:
            if (s_parser.count >= MAX_COMMAND_LENGTH) {
                // 缓冲区溢出,重置
                s_parser.count = 0;
                s_parser.state = PARSE_STATE_IDLE;
                
                // 如果当前字节是帧头,重新开始
                if (data == FRAME_HEADER) {
                    s_parser.buffer[0] = data;
                    s_parser.count = 1;
                    s_parser.state = PARSE_STATE_RECEIVING;
                }
                break;
            }
            
            s_parser.buffer[s_parser.count++] = data;
            
            // 检查标志位
            if (s_parser.count == (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE + FRAME_FLAG_SIZE)) {
                if (data != FRAME_FLAG) {
                    s_parser.count = 0;
                    s_parser.state = PARSE_STATE_IDLE;
                    break;
                }
            }
            
            // 接收完长度字段
            if (s_parser.count == (FRAME_HEADER_SIZE + FRAME_LENGTH_SIZE)) {
                s_parser.expectedLength = ((uint16_t)s_parser.buffer[1] << 8) | s_parser.buffer[2];
                
                // 验证长度合理性
                if (s_parser.expectedLength > (MAX_COMMAND_LENGTH - FRAME_OVERHEAD)) {
                    s_parser.count = 0;
                    s_parser.state = PARSE_STATE_IDLE;
                    break;
                }
            }
            
            // 检查是否接收完整帧
            if (s_parser.count == (FRAME_OVERHEAD + s_parser.expectedLength)) {
                ProcessCompleteFrame();
                
                // 重置状态机
                s_parser.count = 0;
                s_parser.state = PARSE_STATE_IDLE;
            }
            break;
            
        default:
            s_parser.count = 0;
            s_parser.state = PARSE_STATE_IDLE;
            break;
    }
}
