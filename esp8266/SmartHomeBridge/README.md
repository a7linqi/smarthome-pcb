# SmartHomeBridge

这是 ESP8266 的正式串口桥接骨架，包含：

- Wi-Fi station 与 20 秒断线重试；
- 与 STM32 相同的 A5 5A、CRC16/CCITT-FALSE 二进制协议；
- 每 2 秒发送心跳并识别 STM32 回包；
- 解析 24 字节传感器快照和 3 字节设备状态；
- MQTT 自动重连、遗嘱状态、发布、订阅与 Keep Alive；
- MQTT 控制命令转 UART 帧，STM32 应答再发布回 MQTT。

先复制 `bridge_config.example.h` 为 `bridge_config.h`，填写 2.4 GHz Wi-Fi、
Broker 地址、端口和可选账号。`bridge_config.h` 被 Git 忽略，不会上传密码。

## MQTT消息

| Topic | 方向 | Retain | Payload示例 |
| --- | --- | --- | --- |
| `smarthome/device01/sensor` | ESP发布 | 否 | `{"temperature_centi_c":2534,...}` |
| `smarthome/device01/state` | ESP发布 | 是 | `{"mode":0,"light_on":1,"door_open":0}` |
| `smarthome/device01/command` | ESP订阅 | 否 | `{"device":1,"value":1}` |
| `smarthome/device01/ack` | ESP发布 | 否 | `{"sequence":7,"status":0,"device":1,"value":1}` |
| `smarthome/device01/status` | ESP发布/遗嘱 | 是 | `online`或`offline` |

设备编号：`1=灯`、`2=门`、`3=模式`；当前值均为`0`或`1`。

本项目使用 PubSubClient 2.8。该库可以用 QoS 1 订阅 command，也能为遗嘱设置
QoS 1；它的普通 `publish()` 接口只发送 QoS 0。因此 `sensor/state/ack/online`
当前都是 QoS 0。控制是否真正执行，要以 STM32 返回并发布到 `ack` 的业务应答为准。

正式运行时 UART0 用于 STM32 二进制协议，不能在 `Serial` 上插入调试文字。
ESP8266 上电的 ROM 信息使用 74880 波特率，STM32 解析器会跳过不符合帧头的数据。

联调顺序：

1. ESP TX(GPIO1) 接 STM32 PB11(USART3_RX)。
2. ESP RX(GPIO3) 接 STM32 PB10(USART3_TX)。
3. 两板共地，使用 3.3 V 逻辑电平。
4. 烧录 ESP 时避免 STM32 TX 与 USB-TTL TX 同时驱动 ESP RX。
5. 烧录后解除 GPIO0 接地并复位，再启动 STM32。

没有实物时只能完成编译和 Broker 侧模拟，不能据此声称 UART、电源、Wi-Fi 信号和
设备动作已经联调通过。上板后必须逐项验证。
