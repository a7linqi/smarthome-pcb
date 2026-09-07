#ifndef BSP_SENSORS_H
#define BSP_SENSORS_H

#include <stdint.h>
#include "bsp_status.h"

enum {
    BSP_SENSOR_VALID_DHT11  = 1U << 0,
    BSP_SENSOR_VALID_BMP280 = 1U << 1,
    BSP_SENSOR_VALID_ADC    = 1U << 2
};

typedef struct {
    uint32_t timestamp_ms;          /* 采样时刻，HAL_GetTick() 毫秒 */
    uint32_t valid_mask;            /* 哪些传感器本次读取成功，BSP_SENSOR_VALID_xxx 位或 */
    int16_t  temperature_centi_c;   /* 温度，单位 0.01°C（DHT11 或 BMP280 备用） */
    uint16_t humidity_centi_pct;    /* 湿度，单位 0.01%RH（仅 DHT11） */
    uint32_t pressure_pa;           /* 大气压，单位 Pa（仅 BMP280） */
    uint16_t light_raw;             /* 光敏电阻 ADC 原始值，0~4095 */
    uint16_t mq2_raw;               /* MQ-2 烟雾/可燃气体 ADC 原始值 */
    uint16_t mq7_raw;               /* MQ-7 一氧化碳 ADC 原始值 */
    uint16_t mq135_raw;             /* MQ-135 空气质量 ADC 原始值 */
} bsp_sensor_snapshot_t;

bsp_status_t BSP_Sensors_Init(void);
bsp_status_t BSP_Sensors_Read(bsp_sensor_snapshot_t *snapshot);

#endif
