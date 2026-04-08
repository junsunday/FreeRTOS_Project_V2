# 新架构快速参考

## 📋 命令ID速查表

| 命令ID | 宏定义 | 功能 | Payload格式 |
|--------|--------|------|-------------|
| 0x01 | `CMD_LED_CONTROL` | LED状态控制 | [LED状态字节] |
| 0x02 | `CMD_LED_BREATH_START` | 启动呼吸灯 | 无 |
| 0x03 | `CMD_LED_BREATH_STOP` | 停止呼吸灯 | 无 |
| 0x10 | `CMD_SYS_STATUS_QUERY` | 查询系统状态 | 无 |
| 0x11 | `CMD_SYS_RESET` | 系统复位 | 无 |
| 0x20 | `CMD_DATA_PASSTHROUGH` | 数据透传 | [任意64字节] |

## 🔌 输入源标识

| 来源 | 宏定义 | 值 |
|------|--------|-----|
| HID USB | `CMD_SRC_HID` | 0x01 |
| UART串口 | `CMD_SRC_UART` | 0x02 |
| 按钮 | `CMD_SRC_BUTTON` | 0x03 |
| 系统内部 | `CMD_SRC_SYSTEM` | 0xFF |

## 📤 输出通道

| 通道 | 宏定义 | 值 |
|------|--------|-----|
| HID | `CHAN_HID` | 0x01 |
| UART | `CHAN_UART` | 0x02 |
| 两者 | `CHAN_BOTH` | 0x03 |

## 🏗️ 任务优先级

| 任务 | 优先级 | 栈大小 |
|------|--------|--------|
| HidInputAdapter | High | 1024字节 |
| UartInputAdapter | High | 1024字节 |
| CommandDispatcher | High | 1024字节 |
| ResponseManager | High | 1024字节 |
| ButtonInputAdapter | Normal | 512字节 |

## 📨 消息队列

| 队列 | 深度 | 元素大小 | 用途 |
|------|------|----------|------|
| CommandQueueHandle | 32 | sizeof(Command_t) | 输入→分发器 |
| ResponseQueueHandle | 16 | sizeof(Response_t) | 分发器→输出 |

## 🔧 常用API

### 注册新命令处理器
```c
// 在 CommandDispatcher_Init() 中添加
CommandDispatcher_RegisterHandler(CMD_YOUR_COMMAND, YourHandlerFunction);
```

### 发送命令 (从输入适配器)
```c
Command_t cmd;
cmd.source = CMD_SRC_XXX;
cmd.command_id = CMD_XXX;
cmd.payload_len = len;
memcpy(cmd.payload, data, len);
osMessageQueuePut(CommandQueueHandle, &cmd, 0, 10);
```

### 构建响应 (从业务控制器)
```c
void YourHandler(Command_t *cmd, Response_t *resp) {
    resp->status = CMD_STATUS_SUCCESS;
    resp->data[0] = result;
    resp->data_len = 1;
    // target_channel 由分发器自动设置
}
```

## 🐛 调试宏

```c
// 在需要调试的地方添加
printf("[Module] Message\n");

// 查看队列状态
printf("CmdQ: %d/%d\n", 
       osMessageQueueGetCount(CommandQueueHandle),
       osMessageQueueGetCapacity(CommandQueueHandle));

// 查看任务栈水位
printf("Stack: %d\n", uxTaskGetStackHighWaterMark(NULL));
```

## 📝 协议帧格式 (UART)

```
字节偏移:  0    1    2    3    4    5   ...  N-1   N
内容:     AA  LenH LenL  BB  CmdID Status Data... CS
          ↑         ↑         ↑              ↑     ↑
        帧头    长度(大端)  标志           数据  校验和
```

**校验和计算**:
```c
uint8_t checksum = 0;
for (int i = 0; i < total_len - 1; i++) {
    checksum += frame[i];
}
frame[total_len - 1] = checksum;
```

## ⚡ 快速扩展模板

### 添加新业务模块

```c
// YourController.h
#ifndef __YOUR_CONTROLLER_H
#define __YOUR_CONTROLLER_H
#include "CommandDef.h"
void YourController_Init(void);
void YourController_HandleCommand(Command_t *cmd, Response_t *resp);
#endif

// YourController.c
#include "YourController.h"
void YourController_Init(void) {
    // 初始化代码
}
void YourController_HandleCommand(Command_t *cmd, Response_t *resp) {
    // 处理逻辑
    resp->status = CMD_STATUS_SUCCESS;
    resp->data[0] = result;
    resp->data_len = 1;
}

// 在 CommandDispatcher_Init() 中注册
CommandDispatcher_RegisterHandler(CMD_YOUR_CMD, YourController_HandleCommand);
```

### 添加新输入源

```c
// NewInputAdapter.c
#include "InputAdapters.h"
static osMessageQueueId_t s_cmdQueue;

void NewInputAdapter_Init(osMessageQueueId_t cmdQueue) {
    s_cmdQueue = cmdQueue;
}

void NewInputAdapter_Task(void *argument) {
    Command_t cmd;
    for (;;) {
        // 读取原始数据
        uint8_t data[32];
        uint16_t len = ReadNewSource(data);
        
        // 转换为标准命令
        cmd.source = CMD_SRC_NEW;
        cmd.command_id = data[0];
        memcpy(cmd.payload, &data[1], len-1);
        cmd.payload_len = len-1;
        
        // 发送
        osMessageQueuePut(s_cmdQueue, &cmd, 0, 10);
        osDelay(5);
    }
}

// 在 freertos.c 中创建任务
osThreadNew(NewInputAdapter_Task, NULL, &(const osThreadAttr_t){
    .name = "NewInputAdapter",
    .stack_size = 256 * 4,
    .priority = osPriorityHigh
});
```

## 🎯 典型应用场景

### 场景1: PC通过HID控制LED
```
PC发送: [64字节HID报告, 第1字节=0x01(LED控制), 第2字节=LED状态]
  ↓
HidInputAdapter → Command_t(source=HID, id=PASSTHROUGH)
  ↓
CommandDispatcher → 未找到PASSTHROUGH处理器 → 返回错误
```

**改进**: 自定义HID协议
```c
// 在 HidInputAdapter_Task 中解析
if (s_hidBuffer[0] == 0x01) {  // LED控制
    cmd.command_id = CMD_LED_CONTROL;
    cmd.payload[0] = s_hidBuffer[1];
    cmd.payload_len = 1;
}
```

### 场景2: 串口查询系统状态
```
发送: AA 00 01 BB 10 CS
      ↑        ↑  ↑
    帧头   长度  CmdID=SYS_STATUS_QUERY
    
接收: AA 00 0A BB 10 00 [10字节状态数据] CS
                ↑  ↑   ↑
              CmdID OK  数据
```

### 场景3: 按钮触发+双向反馈
```
按下WKUP按钮
  ↓
ButtonInputAdapter → CMD_LED_BREATH_START
  ↓
LedController 启动呼吸灯
  ↓
ResponseManager → 同时发送到HID和UART
  ↓
PC收到通知: "呼吸灯已启动"
```

## ⚠️ 注意事项

1. **不要在业务逻辑中直接操作硬件**
   ```c
   ❌ HAL_GPIO_WritePin(...)
   ✅ LedController_SetState(...)
   ```

2. **USB发送前检查状态**
   ```c
   if (hUsbDeviceFS.dev_state != USBD_STATE_CONFIGURED) {
       return -1;
   }
   ```

3. **队列操作加超时**
   ```c
   osMessageQueuePut(queue, &data, 0, 10);  // ✅ 10ms超时
   osMessageQueuePut(queue, &data, 0, osWaitForever);  // ❌ 可能死锁
   ```

4. **协议解析防溢出**
   ```c
   if (parser.count >= MAX_LENGTH) {
       // 重置状态机
       parser.count = 0;
       parser.state = PARSE_STATE_IDLE;
   }
   ```

## 📊 性能监控

```c
// 定期打印系统状态
void PrintSystemStatus(void) {
    printf("=== System Status ===\n");
    printf("Uptime: %lu seconds\n", osKernelGetTickCount() / 1000);
    printf("USB State: %d\n", hUsbDeviceFS.dev_state);
    printf("Cmd Queue: %d/%d\n", 
           osMessageQueueGetCount(CommandQueueHandle),
           osMessageQueueGetCapacity(CommandQueueHandle));
    printf("Resp Queue: %d/%d\n",
           osMessageQueueGetCount(ResponseQueueHandle),
           osMessageQueueGetCapacity(ResponseQueueHandle));
}
```

---

**提示**: 将此文件加入书签,开发时快速查阅!
