# STM32 + ESP8266 智能家居系统

本项目是对旧版“STM32F103 + ESP8266/机智云智能家居”的重新开发。
目标硬件以成品传感器 PCB 为底板，STM32F103 负责实时采集和设备控制，
板载 ESP8266 负责 Wi-Fi、MQTT 和手机端通信。

正式工程路径：`F:\Smart`。

## 最终架构

- STM32F103C8T6：传感器、OLED、按键、蜂鸣器、电机及本地自动控制
- ESP8266：Wi-Fi、MQTT、远程控制和设备状态转发
- STM32 与 ESP8266：UART 115200 bit/s
- STM32 软件：FreeRTOS
- ESP8266 软件：Arduino Core 3.1.2

## 当前进度

- [x] 读取旧工程、原理图和 BOM
- [x] 建立初版硬件引脚表
- [x] 确定双 MCU 重构架构
- [ ] 实物通电与引脚验证
- [x] STM32 最小工程与板级驱动代码（待实物自检）
- [x] FreeRTOS 任务划分与队列通信
- [x] STM32/ESP8266 串口协议代码（待上板联调）
- [x] ESP8266 MQTT 客户端代码（待上板联网验证）
- [x] 本地 Broker 与电脑端 MQTT 测试流程
- [ ] 实物联调、故障恢复验证与演示视频

## 开发原则

1. 先验证供电、下载和单个外设，再上 RTOS 与联网。
2. 引脚表中标为“待实测”的项目，在万用表或最小程序确认前不直接驱动。
3. 每完成一个模块保留测试结果，保证简历中的描述可复现、可解释。

详细计划见 [docs/BUILD_PLAN.md](docs/BUILD_PLAN.md)，初版引脚表见
[docs/PINMAP.md](docs/PINMAP.md)。串口、TCP/IP、MQTT、CAN 的学习与面试复习持续记录在
[docs/IOT_INTERVIEW_NOTES.md](docs/IOT_INTERVIEW_NOTES.md)。

ESP8266 正式程序与说明见
[esp8266/SmartHomeBridge](esp8266/SmartHomeBridge)，本地 MQTT 实验说明见
[tools/MQTT_LAB.md](tools/MQTT_LAB.md)。

## 当前可交付边界

仓库将软件实现、编译检查和电脑端 Broker 测试做到可复现。由于当前没有智能家居
实物，以下结论仍待上板：STM32 与 ESP8266 串口电气连接、真实传感器数据、Wi-Fi
断线恢复、远程命令驱动实际设备以及长时间稳定性。简历和答辩必须按这个边界描述。
