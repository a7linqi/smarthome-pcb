# Smart 项目交接（2026-09-07）

## 项目边界
- 当前继续 F:/Smart 智能家居，联网模块按用户确认使用 ESP8266，Arduino 开发。
- F:/jiaohua 是用户另外焊接的浇花项目，不能套用其 PA2/PA3 引脚。
- 用户报告 GPIO0 拉低后已成功连接并下载；本轮尚未亲自验证智能家居板联网。
- 根 README 和 BUILD_PLAN 已更新为 ESP8266/Arduino；esp32 目录为早期规划留存。

## 已有代码
- STM32 已有 Sensor、Display、Control、Alarm 四个任务；用户报告已完成底层和 FreeRTOS。
- BSP 已有 UART3 环形缓冲、CRC16 二进制协议；APP_Init 已创建串口任务和通信队列。
- Smart 引脚文档：PB10 TX、PB11 RX；实际连接仍需与板卡核对。
- esp8266/SmokeTest 是原有扫描和回显程序。

## 本轮新增
- esp8266/WiFiConnect：独立 station 联网验证、20 秒重试、状态日志。
- wifi_config.h 本地填写凭据并由 .gitignore 排除；模板可重建该文件。
- 使用本机 ESP8266 core 3.1.2、esp8266:esp8266:generic 成功编译。
- 未烧录、未测试实际联网和断线恢复。此阶段 UART 为文本日志，不是桥接协议。
- 新增 esp8266/SmartHomeBridge：二进制收发、CRC16、心跳、传感器/设备状态解析和控制入口。
- STM32 新增 UartTask：传感器/设备状态上报、心跳回复、控制命令投递和 ACK；OLED 显示 ESP 链路状态。
- ESP8266 正式桥接程序已编译通过；STM32 使用 Keil ARMCC 5.06 编译通过（0 Error、0 Warning），尚需上板联调。

## 下一步
1. 用户本机填写 Wi-Fi，上传，确认 Connected/IP 和断网恢复。
2. 按 Smart 板核对 UART3 接线，验证心跳、传感器上报及设备状态。
3. 按 UART_PROTOCOL.md 验证控制命令与 ACK；联网状态不得影响本地控制。
4. 确认 MQTT 服务和手机端方案后实现发布、订阅及重连。

## 注意
工作区已有用户修改，继续前检查实际文件，不覆盖现有 STM32 工作。
