# FreeRTOS 多源通信任务架构说明

## 1. 架构概述

本项目的任务架构采用**三层分层设计**,实现了输入源、业务逻辑和输出通道的完全解耦,便于扩展和维护。

### 架构图

```
┌──────────────────────────────────────────────────┐
│           应用层 (Application Layer)              │
│  ┌─────────────────┐    ┌─────────────────────┐  │
│  │ CommandDispatcher│───▶│  ResponseManager    │  │
│  │  (命令分发器)     │    │   (响应管理器)       │  │
│  └────────┬─────────┘    └──────────┬──────────┘  │
│           │                         │              │
└───────────┼─────────────────────────┼──────────────┘
            │ 统一消息队列             │ 统一消息队列
┌───────────┼─────────────────────────┼──────────────┐
│           ▼                         │              │
│      业务逻辑层 (Business Layer)     │              │
│  ┌─────────────────┐               │              │
│  │ LedController   │               │              │
│  │  (LED控制器)     │               │              │
│  ├─────────────────┤               │              │
│  │ SystemMonitor   │               │              │
│  │  (系统监控器)    │               │              │
│  └─────────────────┘               │              │
└───────────┬─────────────────────────┼──────────────┘
            │ 标准化命令               │
┌───────────┼─────────────────────────┼──────────────┐
│           ▼                         │              │
│     输入适配层 (Input Adapter Layer) │              │
│  ┌──────────┐ ┌──────────┐ ┌──────┴──────┐       │
│  │HidAdapter│ │UartAdapter│ │ButtonAdapter│       │
│  │(HID输入)  │ │(串口输入)  │ │(按钮输入)    │       │
│  └──────────┘ └──────────┘ └─────────────┘       │
└──────────────────────────────────────────────────┘
```

## 2. 核心组件说明

### 2.1 输入适配层 (Input Adapter Layer)

**职责**: 将不同输入源的原始数据转换为统一的 `Command_t` 结构

#### HidInputAdapter
- **文件**: `HidInputAdapter.c/h`
- **功能**: 接收USB HID数据,转换为透传命令
- **任务优先级**: High
- **特点**: 
  - 通过 `CUSTOM_HID_OutEvent_FS` 回调接收数据
  - 默认使用 `CMD_DATA_PASSTHROUGH` 命令ID

#### UartInputAdapter
- **文件**: `UartInputAdapter.c/h`
- **功能**: 接收串口数据,解析协议帧,提取命令
- **任务优先级**: High
- **协议格式**:
  ```
  [帧头AA][长度高][长度低][标志BB][命令ID][数据...][校验和]
  ```
- **特点**:
  - 包含完整的协议解析状态机
  - 自动验证帧完整性和校验和
  - 支持错误恢复和重新同步

#### ButtonInputAdapter
- **文件**: `ButtonInputAdapter.c/h`
- **功能**: 轮询按钮状态,检测按下事件
- **任务优先级**: Normal
- **特点**:
  - 软件消抖(50ms)
  - 触发 `CMD_LED_BREATH_START` 命令

### 2.2 应用层 (Application Layer)

#### CommandDispatcher (命令分发器)
- **文件**: `CommandDispatcher.c/h`
- **功能**: 
  - 从命令队列接收 `Command_t`
  - 根据 `command_id` 路由到对应的业务处理器
  - 生成 `Response_t` 并发送到响应队列
- **任务优先级**: High
- **特点**:
  - 支持动态注册命令处理器 (`CommandDispatcher_RegisterHandler`)
  - 未知命令自动返回错误响应
  - 根据命令来源自动设置响应通道

#### ResponseManager (响应管理器)
- **文件**: `ResponseManager.c/h`
- **功能**: 
  - 从响应队列接收 `Response_t`
  - 根据 `target_channel` 选择输出通道(HID/UART/Both)
  - 封装各通道的协议细节
- **任务优先级**: High
- **特点**:
  - UART输出自动构建协议帧
  - HID输出检查USB设备状态
  - 统一的错误处理

### 2.3 业务逻辑层 (Business Layer)

#### LedController (LED控制器)
- **文件**: `LedController.c/h`
- **功能**:
  - 处理LED控制命令 (`CMD_LED_CONTROL`)
  - 管理呼吸灯效果 (`CMD_LED_BREATH_START/STOP`)
  - 封装定时器和PWM操作
- **特点**:
  - 纯业务逻辑,不依赖具体输入源
  - 通过命令参数控制行为
  - 返回执行结果到响应管理器

#### SystemMonitor (系统监控器)
- **文件**: `SystemMonitor.c/h`
- **功能**:
  - 处理系统状态查询 (`CMD_SYS_STATUS_QUERY`)
  - 管理USB设备初始化
  - 收集系统运行信息
- **特点**:
  - USB初始化仅执行一次
  - 提供标准化的状态数据结构

## 3. 数据流示例

### 3.1 HID控制LED流程

```
PC发送HID数据
    ↓
CUSTOM_HID_OutEvent_FS 回调
    ↓
HidInputAdapter_ProcessData()
    ↓
构建 Command_t (source=CMD_SRC_HID, command_id=CMD_DATA_PASSTHROUGH)
    ↓
osMessageQueuePut(CommandQueueHandle)
    ↓
CommandDispatcherTask 接收命令
    ↓
查找处理器 (未找到,返回错误) 或 路由到对应业务模块
    ↓
构建 Response_t
    ↓
osMessageQueuePut(ResponseQueueHandle)
    ↓
ResponseManagerTask 接收响应
    ↓
根据 target_channel 发送到 HID/UART
```

### 3.2 串口控制LED流程

```
串口接收字节
    ↓
HAL_UART_RxCpltCallback 中断回调
    ↓
osMessageQueuePut(CommandQueueHandle, &rxByte)
    ↓
UartAdapterTask 接收字节
    ↓
协议解析状态机处理
    ↓
帧接收完成后构建 Command_t (source=CMD_SRC_UART, command_id=从payload提取)
    ↓
osMessageQueuePut(CommandQueueHandle)
    ↓
CommandDispatcherTask 接收命令
    ↓
路由到 LedController_HandleCommand()
    ↓
执行 LED 控制逻辑
    ↓
构建 Response_t (status, data)
    ↓
osMessageQueuePut(ResponseQueueHandle)
    ↓
ResponseManagerTask 接收响应
    ↓
构建UART协议帧并发送
```

### 3.3 按钮触发呼吸灯流程

```
ButtonAdapterTask 轮询GPIO
    ↓
检测到上升沿 + 消抖确认
    ↓
构建 Command_t (source=CMD_SRC_BUTTON, command_id=CMD_LED_BREATH_START)
    ↓
osMessageQueuePut(CommandQueueHandle)
    ↓
CommandDispatcherTask 接收命令
    ↓
路由到 LedController_HandleCommand()
    ↓
调用 LedController_StartBreath()
    ↓
启动定时器和PWM
    ↓
构建 Response_t
    ↓
osMessageQueuePut(ResponseQueueHandle)
    ↓
ResponseManagerTask 发送到 HID + UART
```

## 4. 扩展指南

### 4.1 添加新的输入源

**步骤**:
1. 在 `InputAdapters.h` 中添加新的适配器接口
2. 创建 `XxxInputAdapter.c` 实现文件
3. 在任务函数中将原始数据转换为 `Command_t`
4. 发送到 `CommandQueueHandle`
5. 在 `freertos.c` 中创建新任务并初始化

**示例**: 添加SPI输入适配器
```c
// SpiInputAdapter.c
void SpiInputAdapter_Task(void *argument) {
    Command_t cmd;
    uint8_t spiData[32];
    
    for (;;) {
        if (SPI_DataReady()) {
            SPI_Read(spiData, 32);
            
            cmd.source = CMD_SRC_SPI;
            cmd.command_id = spiData[0];
            memcpy(cmd.payload, &spiData[1], 31);
            cmd.payload_len = 31;
            
            osMessageQueuePut(CommandQueueHandle, &cmd, 0, 10);
        }
        osDelay(5);
    }
}
```

### 4.2 添加新的业务功能

**步骤**:
1. 在 `CommandDef.h` 中定义新的命令ID
2. 创建业务控制器 `XxxController.c/h`
3. 实现处理函数 `void XxxController_HandleCommand(Command_t *cmd, Response_t *resp)`
4. 在 `CommandDispatcher_Init()` 中注册处理器

**示例**: 添加电机控制
```c
// CommandDef.h
#define CMD_MOTOR_SPEED_SET   0x30

// MotorController.c
void MotorController_HandleCommand(Command_t *cmd, Response_t *resp) {
    if (cmd->command_id == CMD_MOTOR_SPEED_SET) {
        uint16_t speed = (cmd->payload[0] << 8) | cmd->payload[1];
        MOTOR_SetSpeed(speed);
        
        resp->data[0] = (speed >> 8) & 0xFF;
        resp->data[1] = speed & 0xFF;
        resp->data_len = 2;
    }
}

// CommandDispatcher.c
CommandDispatcher_RegisterHandler(CMD_MOTOR_SPEED_SET, MotorController_HandleCommand);
```

### 4.3 添加新的输出通道

**步骤**:
1. 在 `CommandDef.h` 中定义新的通道宏 (如 `#define CHAN_CAN 0x04`)
2. 在 `ResponseManager.c` 中添加发送函数 `SendToCAN()`
3. 在 `ResponseManagerTask()` 中添加通道判断分支

**示例**: 添加CAN总线输出
```c
static int8_t SendToCAN(Response_t *resp) {
    CAN_TxHeaderTypeDef txHeader;
    uint8_t canData[8];
    
    // 构建CAN数据包
    canData[0] = resp->command_id;
    canData[1] = resp->status;
    memcpy(&canData[2], resp->data, 6);
    
    // 发送
    HAL_CAN_AddTxMessage(&hcan, &txHeader, canData, &mailbox);
    return 0;
}
```

## 5. 调试技巧

### 5.1 查看命令流

在 `CommandDispatcherTask` 中添加日志:
```c
printf("[Dispatcher] Received cmd: src=0x%02X, id=0x%02X, len=%d\n", 
       cmd.source, cmd.command_id, cmd.payload_len);
```

### 5.2 查看响应流

在 `ResponseManagerTask` 中添加日志:
```c
printf("[ResponseMgr] Sending resp: cmd=0x%02X, status=0x%02X, chan=0x%02X\n",
       resp->command_id, resp->status, resp->target_channel);
```

### 5.3 监控任务栈使用

```c
// 在各任务中添加
printf("Task stack watermark: %d\n", uxTaskGetStackHighWaterMark(NULL));
```

## 6. 注意事项

### 6.1 避免的问题

❌ **禁止在业务逻辑层直接操作硬件**
```c
// 错误示例
void LedController_HandleCommand(...) {
    HAL_GPIO_WritePin(...); // ❌ 应该调用内部函数
}

// 正确示例
void LedController_HandleCommand(...) {
    UpdateLEDState(ledState); // ✅ 封装硬件操作
}
```

❌ **禁止多个任务重复初始化外设**
```c
// 错误: 在多个地方调用 MX_USB_DEVICE_Init()
// 正确: 仅在 SystemMonitor_Init() 中调用一次
```

❌ **禁止跨层调用**
```c
// 错误: 输入适配器直接调用业务逻辑
HidAdapterTask() {
    LedController_SetState(...); // ❌
}

// 正确: 通过命令队列通信
HidAdapterTask() {
    osMessageQueuePut(CommandQueueHandle, &cmd, ...); // ✅
}
```

### 6.2 性能优化建议

1. **消息队列大小**: 根据实际吞吐量调整,避免溢出
2. **任务优先级**: 输入适配器和分发器设为High,确保实时性
3. **协议解析**: 使用状态机避免阻塞,单字节快速处理
4. **内存管理**: 避免在任务中动态分配内存

## 7. 与旧架构对比

| 特性 | 旧架构 | 新架构 |
|------|--------|--------|
| 任务数量 | 4个混杂任务 | 7个专职任务 |
| 职责划分 | 模糊,多处重复 | 清晰,单一职责 |
| 扩展性 | 需修改多处代码 | 仅需添加新模块 |
| 耦合度 | 高,直接调用 | 低,消息队列通信 |
| 可维护性 | 差,逻辑分散 | 好,模块化设计 |
| 调试难度 | 困难 | 简单,日志集中 |

## 8. 总结

新架构通过**分层设计**和**消息驱动**实现了:
- ✅ 输入源与业务逻辑解耦
- ✅ 业务逻辑与输出通道解耦
- ✅ 统一的命令/响应机制
- ✅ 易于扩展新功能
- ✅ 清晰的调试路径

遵循此架构,后续添加新功能只需:
1. 新增输入适配器(如需新输入源)
2. 新增业务控制器(如需新业务)
3. 注册命令处理器
4. 无需修改现有代码
