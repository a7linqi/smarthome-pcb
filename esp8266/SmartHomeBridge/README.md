# SmartHomeBridge

这是 ESP8266 的正式串口桥接骨架，包含：

- Wi-Fi station 与 20 秒断线重试；
- 与 STM32 相同的 A5 5A、CRC16/CCITT-FALSE 二进制协议；
- 每 2 秒发送心跳并识别 STM32 回包；
- 解析 24 字节传感器快照和 3 字节设备状态；
- 预留控制命令发送和控制应答处理入口。

先复制 `bridge_config.example.h` 为 `bridge_config.h` 并在本机填写 2.4 GHz Wi-Fi。
当前仓库已放置空配置，可直接编译。

正式运行时 UART0 用于 STM32 二进制协议，不能在 `Serial` 上插入调试文字。
ESP8266 上电的 ROM 信息使用 74880 波特率，STM32 解析器会跳过不符合帧头的数据。

联调顺序：

1. ESP TX(GPIO1) 接 STM32 PB11(USART3_RX)。
2. ESP RX(GPIO3) 接 STM32 PB10(USART3_TX)。
3. 两板共地，使用 3.3 V 逻辑电平。
4. 烧录 ESP 时避免 STM32 TX 与 USB-TTL TX 同时驱动 ESP RX。
5. 烧录后解除 GPIO0 接地并复位，再启动 STM32。

`sendControlCommand()` 将在 MQTT 阶段由消息回调调用。目前不自动发送控制命令，
避免上电后意外改变灯或门。
