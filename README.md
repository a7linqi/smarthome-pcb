# STM32 + ESP32-S3 智能家居系统

本项目是对旧版“STM32F103 + ESP8266/机智云智能家居”的重新开发。
目标硬件以成品传感器 PCB 为底板，STM32F103 负责实时采集和设备控制，
外接 ESP32-S3 负责 Wi-Fi、MQTT 和手机端通信。

正式工程路径：`F:\Smart`。

## 最终架构

- STM32F103C8T6：传感器、OLED、按键、蜂鸣器、电机及本地自动控制
- ESP32-S3：Wi-Fi、MQTT、远程控制和设备状态转发
- STM32 与 ESP32-S3：UART 115200 bit/s
- STM32 软件：FreeRTOS
- ESP32 软件：ESP-IDF

## 当前进度

- [x] 读取旧工程、原理图和 BOM
- [x] 建立初版硬件引脚表
- [x] 确定双 MCU 重构架构
- [ ] 实物通电与引脚验证
- [ ] STM32 最小工程和板级自检
- [ ] FreeRTOS 任务划分
- [ ] STM32/ESP32 串口协议
- [ ] ESP-IDF MQTT 客户端
- [ ] 联调、故障恢复与演示材料

## 开发原则

1. 先验证供电、下载和单个外设，再上 RTOS 与联网。
2. 引脚表中标为“待实测”的项目，在万用表或最小程序确认前不直接驱动。
3. 每完成一个模块保留测试结果，保证简历中的描述可复现、可解释。

详细计划见 [docs/BUILD_PLAN.md](docs/BUILD_PLAN.md)，初版引脚表见
[docs/PINMAP.md](docs/PINMAP.md)。
