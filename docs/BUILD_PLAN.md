# 快速重构计划

## 阶段 0：硬件基线

- 确认成品板输入电压、3.3 V 和 5 V 电源轨。
- 确认 STM32 型号、BOOT0 状态和 SWD 下载接口。
- 烧录 LED/串口自检程序，证明板卡可开发。
- 逐项确认 OLED、DHT11、BMP280、ADC、蜂鸣器、按键和步进电机。

验收标准：所有板载模块都有独立、可重复运行的自检结果。

## 阶段 1：STM32 裸机板级支持包

先建立可测的 BSP，不立刻复制旧项目业务逻辑：

- `bsp_led`
- `bsp_key`
- `bsp_buzzer`
- `bsp_oled`
- `bsp_dht11`
- `bsp_bmp280`
- `bsp_adc_sensor`
- `bsp_stepper`
- `bsp_uart_bridge`

验收标准：每个驱动均有返回值、超时和错误状态，主循环能够输出自检报告。

## 阶段 2：FreeRTOS 应用

计划任务：

| 任务 | 职责 | 初始周期 |
| --- | --- | --- |
| SensorTask | 采集温湿度、气压、光照和气体数据 | 1000 ms |
| ControlTask | 自动/手动控制与阈值判断 | 50 ms |
| DisplayTask | OLED 页面刷新 | 200 ms |
| AlarmTask | 蜂鸣器报警状态机 | 50 ms |
| UartTask | 与 ESP8266 收发数据帧 | 事件驱动 |

共享传感器快照由互斥锁保护；按键使用 EXTI + 二值信号量；UART 接收使用
中断/环形缓冲区，不在中断中解析协议。

## 阶段 3：ESP8266 与 MQTT

- ESP8266 Arduino Core 建立 Wi-Fi station 和自动重连。
- UART 接收 STM32 状态帧并校验。
- MQTT 上报传感器和设备状态。
- MQTT 控制命令转换为 UART 控制帧。
- 网络不可用时 STM32 本地自动控制继续工作。

## 阶段 4：联调与简历证据

- 保存 UART 协议文档和抓包日志。
- 记录 FreeRTOS 任务、优先级、栈余量和 CPU 占用。
- 拍摄本地自动控制、手机远程控制、断网恢复演示视频。
- 整理硬件照片、系统架构图、故障闭环和 README。

## 第一轮实物操作

1. 暂时不要同时给成品板的多个 USB 口供电。
2. 用万用表确认板上 5 V、3.3 V 与 GND。
3. 使用 ST-Link 连接 SWDIO、SWCLK、GND、3.3 V 参考电压。
4. 读取芯片 ID 并全片擦除前先备份现有固件（若需要保留卖家程序）。
5. 用最小 LED/USART 程序验证下载和运行。

注意：烧录 ESP8266 时，USB-TTL TX 和 STM32 TX 不应同时驱动 ESP8266 RX。
