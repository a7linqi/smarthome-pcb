# 成品板初版引脚表

本表依据旧工程 GPIO 定义、原理图文字层和 BOM 整理。最终以实物连通性测试为准。

| 功能 | STM32 引脚 | 外设/接口 | 状态 |
| --- | --- | --- | --- |
| 光照 ADC | PA4 / ADC1_CH4 | GL7516 | 旧代码确认 |
| MQ-7 ADC | PA5 / ADC1_CH5 | MQ-7 | 旧代码确认 |
| MQ-135 ADC | PA6 / ADC1_CH6 | MQ-135 | 旧代码确认 |
| MQ-2 ADC | PA7 / ADC1_CH7 | MQ-2 | 旧代码确认 |
| BMP280 SCL | PB6 | 软件 I2C | 旧代码确认 |
| BMP280 SDA | PB7 | 软件 I2C | 旧代码确认 |
| DHT11 DATA | PB14 | 单总线 | 旧代码确认 |
| OLED SCL | PB12 | 软件 I2C | 旧代码确认 |
| OLED SDA | PB13 | 软件 I2C | 旧代码确认 |
| 照明 LED | PB0 | GPIO 输出 | 旧代码确认，低有效待实测 |
| 蜂鸣器 | PC13 | GPIO 输出 | 旧代码确认，电平极性待实测 |
| 步进相位 1 | PA11 | ULN2003A | 旧代码确认 |
| 步进相位 2 | PA12 | ULN2003A | 旧代码确认 |
| 步进相位 3 | PA15 | ULN2003A | 旧代码确认；占用 JTAG 引脚 |
| 步进相位 4 | PB3 | ULN2003A | 旧代码确认；占用 JTAG 引脚 |
| 按键 KEY0 | PB9 | GPIO 输入 | 旧代码确认，低有效 |
| 按键 KEY1 | PB5 | GPIO 输入 | 旧代码确认，低有效 |
| 按键 KEY2 | PB8 | GPIO 输入 | 旧代码确认，低有效 |
| 按键 WK_UP | PA0 | GPIO 输入 | 旧代码确认，低有效 |
| ESP8266/桥接 TX | PB10 / USART3_TX | UART3 | 旧代码确认 |
| ESP8266/桥接 RX | PB11 / USART3_RX | UART3 | 旧代码确认 |
| 调试串口 TX | PA9 / USART1_TX | 板上排针 | 原理图标注 |
| 调试串口 RX | PA10 / USART1_RX | 板上排针 | 原理图标注 |

## 需要优先核实

- 成品板图片中的 `UART3` 排针是否直接连接 PB10/PB11，以及排针脚序。
- 板载 ESP8266是否与 UART3 排针并联；若并联，连接 ESP32-S3 前需拔下 ESP8266。
- OLED 实际地址通常为 `0x3C`，但应在 I2C 扫描中确认。
- PA15/PB3 默认属于 JTAG；软件需要关闭 JTAG、保留 SWD 后才能驱动步进电机。
- MQ 模块模拟输出是否经过分压，必须确认不会超过 STM32 的 3.3 V ADC 上限。
