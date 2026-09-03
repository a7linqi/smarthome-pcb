# STM32 与 ESP32-S3 串口协议

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
