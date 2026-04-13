/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : CommandDef.h
  * @brief          : 统一命令定义和数据结构
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __COMMAND_DEF_H
#define __COMMAND_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "FReeRTOS.h"
/* ========== 命令来源定义 ========== */
#define CMD_SRC_HID       0x01   // HID USB输入
#define CMD_SRC_UART      0x02   // 串口输入
#define CMD_SRC_BUTTON    0x03   // 按钮输入
#define CMD_SRC_SYSTEM    0xFF   // 系统内部命令

/* ========== 命令ID定义 ========== */
// LED控制命令
#define CMD_LED_CONTROL         0x01
#define CMD_LED_BREATH_START    0x02
#define CMD_LED_BREATH_STOP     0x03

// MPU6050 传感器数据命令
#define CMD_MPU6050_DATA        0x04

// 系统监控命令
#define CMD_SYS_STATUS_QUERY    0x10
#define CMD_SYS_RESET           0x11

// 数据透传命令
#define CMD_DATA_PASSTHROUGH    0x20

// 保留扩展
#define CMD_RESERVED            0xFF
#define CMD_OLED_CTRL           0xAA

/* ========== 命令状态定义 ========== */
#define CMD_STATUS_SUCCESS      0x00
#define CMD_STATUS_INVALID      0x01
#define CMD_STATUS_BUSY         0x02
#define CMD_STATUS_ERROR        0x03

/* ========== 统一命令结构 ========== */
#define CMD_PAYLOAD_MAX_LEN     64

typedef struct {
    uint8_t source;                     // 命令来源
    uint8_t command_id;                 // 命令ID
    uint8_t payload[CMD_PAYLOAD_MAX_LEN]; // 命令数据
    uint16_t payload_len;               // 数据长度
    uint32_t timestamp;                 // 时间戳(可选,用于调试)
} Command_t;

/* ========== 响应结构 ========== */
typedef struct {
    uint8_t command_id;                 // 对应的命令ID
    uint8_t status;                     // 执行状态
    uint8_t data[CMD_PAYLOAD_MAX_LEN];  // 响应数据
    uint16_t data_len;                  // 数据长度
    uint8_t target_channel;             // 目标输出通道
} Response_t;

typedef struct {
    float accel[3];
    float gyro[3];
    float angle[3];
    float temperature;
} MPU6050Frame_t;

/* ========== 输出通道定义 ========== */
#define CHAN_HID        0x01
#define CHAN_UART       0x02
#define CHAN_BOTH       0x03

/* ========== 内存管理宏(用于调试) ========== */
#ifdef DEBUG_MEMORY
#define CMD_MALLOC()    debug_malloc(sizeof(Command_t), __FILE__, __LINE__)
#define CMD_FREE(ptr)   debug_free(ptr, __FILE__, __LINE__)
#define RESP_MALLOC()   debug_malloc(sizeof(Response_t), __FILE__, __LINE__)
#define RESP_FREE(ptr)  debug_free(ptr, __FILE__, __LINE__)
#else
#define CMD_MALLOC()    pvPortMalloc(sizeof(Command_t))
#define CMD_FREE(ptr)   vPortFree(ptr)
#define RESP_MALLOC()   pvPortMalloc(sizeof(Response_t))
#define RESP_FREE(ptr)  vPortFree(ptr)
#endif

/**
  * @brief  调试版内存分配(记录分配位置)
  */
#ifdef DEBUG_MEMORY
void* debug_malloc(size_t size, const char *file, int line);
void debug_free(void *ptr, const char *file, int line);
void memory_stats_print(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __COMMAND_DEF_H */
