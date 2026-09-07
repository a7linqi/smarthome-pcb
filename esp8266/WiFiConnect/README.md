# 智能家居：ESP8266 联网验证

使用 Arduino ESP8266 core 3.1.2，自带 ESP8266WiFi，无需额外第三方库。
这是独立的 Wi-Fi 验证程序，尚未包含 STM32 桥接或 MQTT。

1. 打开 WiFiConnect.ino，在 wifi_config.h 填写 2.4 GHz Wi-Fi 名称和密码。
   如果文件不存在，复制 wifi_config.example.h 为 wifi_config.h。
2. 沿用已验证能下载的 ESP8266 板型和 Flash 配置，编译上传。
3. 下载后解除 GPIO0 接地并复位，在 115200 波特率串口监视器查看输出。
4. 出现 `[WiFi] Connected. IP=...` 表示已获取地址。
5. 关闭热点/路由器，观察 Disconnected；重新打开，验证自动恢复并再次打印 IP。
6. 错误密码或路由器不可用时，每 20 秒重试，不在无限 while 中等待。

此程序 UART0 输出文本日志，仅用于 USB-TTL 验证，不要直接作为 STM32 二进制协议固件。
ESP8266 TX 接 USB-TTL RX，双方共地，使用 3.3V 电平。
避免 USB-TTL TX 与 STM32 TX 同时驱动 ESP RX。

Smart 的已有引脚表使用 PB10/USART3_TX、PB11/USART3_RX，需结合智能家居实物核对；
另一个浇花项目的 PA2/PA3 接线不适用于此处。

实际联网和断线恢复结果需上板验证，程序不打印密码。
