# 应用层开发接口

应用入口是 `App/Src/app_main.c` 中的 `APP_Init()`。在这里创建任务、队列、
互斥锁与信号量。

建议任务：

- SensorTask：每 1 s 调用 `BSP_Sensors_Read()`，通过队列发布快照。
- ControlTask：每 50 ms 处理自动/手动模式和执行器命令。
- DisplayTask：修改 OLED显存后调用 `BSP_OLED_Refresh()`。
- AlarmTask：调用 `BSP_Buzzer_SetMode()`，每 10–50 ms 调用 Process。
- UartTask：读取 UART环形缓冲区并交给 `BSP_Protocol_Feed()`。

周期任务使用 `vTaskDelayUntil()`，不要使用忙等待。DHT11读取本身需要严格微秒时序，
调用期间应避免被较长的高优先级任务打断。

MQ模块目前只提供 ADC原始值，因为 MQ浓度换算依赖负载电阻、预热时间和现场标定。
在没有完成标定前，界面与 MQTT字段应明确命名为 `raw`，不要标成 ppm。
