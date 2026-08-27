#ifndef __APP_H
#define __APP_H

#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"

/* ---- 共享数据结构 ---- */
typedef struct {
    uint8_t  temp;       // 温度
    uint8_t  humi;       // 湿度
    uint16_t light;      // 光照 (0-4095)
    uint8_t  mode;       // 0=手动, 1=自动
    uint8_t  door;       // 0=关, 1=开
    uint8_t  led;        // 0=关, 1=开
} SmartHome_Data_t;

/* ---- 全局变量（在 freertos.c 中定义） ---- */
extern SmartHome_Data_t data_t;
extern SemaphoreHandle_t dataMutex;
extern SemaphoreHandle_t keySem;

/* ---- 任务函数声明 ---- */
void SensorTask(void *params);
void ControlTask(void *params);
void DisplayTask(void *params);
void AlarmTask(void *params);
void UartTask(void *params);

#endif /* __APP_H */
