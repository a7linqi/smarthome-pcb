# 应用层入口

从 `App/Src/app_main.c` 的 `APP_Init()` 开始编写应用层：创建 Sensor、Control、
Display、Alarm 和 Uart任务，以及任务间的队列、互斥锁和信号量。

应用层只调用 `BSP/Inc` 暴露的接口，不直接访问 GPIO寄存器或 HAL句柄。这样可将
业务逻辑与硬件驱动分离，也方便面试时解释分层设计。
