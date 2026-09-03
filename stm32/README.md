# STM32 固件

目标平台：STM32F103C8T6 + FreeRTOS。

在完成实物引脚和供电验证后创建工程。优先使用 STM32CubeMX 生成时钟、GPIO、
ADC、UART、FreeRTOS 初始化代码，再把板级驱动和应用逻辑分层加入。
