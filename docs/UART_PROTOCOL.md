# STM32 与 ESP8266 串口协议

串口参数：115200 bit/s、8 数据位、无校验、1 停止位。

| 字段 | 字节数 | 说明 |
| --- | ---: | --- |
| SOF | 2 | 固定 `A5 5A` |
| Version | 1 | 当前为 1 |
| Type | 1 | 消息类型 |
| Sequence | 1 | 序号，用于请求/应答匹配 |
| Flags | 1 | 当前保留为 0 |
| Length | 2 | Payload长度，小端序 |
| Payload | 0–96 | 二进制负载 |
| CRC16 | 2 | CRC-16/CCITT-FALSE，小端序 |

消息类型定义在 `BSP/Inc/bsp_protocol.h`。解析器逐字节工作，可直接消费
`BSP_UartBridge_Read()` 读出的数据；只有返回 `BSP_OK` 时才得到一帧完整消息。

多字节整数统一使用小端序。传感器数值尽量使用定点整数，例如温度以 0.01 ℃、
湿度以 0.01 %RH、气压以 Pa 表示，避免 MCU之间传输浮点数。

## Payload 定义

| 消息 | 方向 | Payload |
| --- | --- | --- |
| HEARTBEAT `0x01` | ESP→STM→ESP | ESP 运行毫秒 `u32`，Wi-Fi 在线 `u8`；STM 原样回复且 sequence 不变 |
| SENSOR_REPORT `0x10` | STM→ESP | 24 字节，见下表 |
| DEVICE_STATE `0x11` | STM→ESP | 模式 `u8`、灯 `u8`、门 `u8`，布尔量使用 0/1 |
| CONTROL_COMMAND `0x20` | ESP→STM | 设备 `u8`、目标值 `u8` |
| CONTROL_ACK `0x21` | STM→ESP | 状态 `u8`、设备 `u8`、实际/请求值 `u8`；sequence 与命令相同 |

设备编号：灯 `1`、门 `2`、模式 `3`。目标值当前只能为 `0` 或 `1`。
ACK 状态：成功 `0`、参数无效 `1`、设备忙 `2`。

### SENSOR_REPORT（24 字节）

| 偏移 | 类型 | 内容 |
| ---: | --- | --- |
| 0 | `u32` | STM32 采样时间，ms |
| 4 | `u32` | 传感器有效位掩码 |
| 8 | `i16` | 温度，0.01 ℃ |
| 10 | `u16` | 湿度，0.01 %RH |
| 12 | `u32` | 气压，Pa |
| 16 | `u16` | 光照 ADC |
| 18 | `u16` | MQ-2 ADC |
| 20 | `u16` | MQ-7 ADC |
| 22 | `u16` | MQ-135 ADC |

正式通信使用二进制 `Serial.write()`；不得在 UART0 混入 `Serial.print()` 日志。
ESP8266 的启动 ROM 输出不是本协议帧，解析器会继续寻找下一个 `A5 5A`。
STM32 收到心跳后认为链路在线；连续 5 秒未收到新心跳时，OLED 显示 `ESP:OFF`。
