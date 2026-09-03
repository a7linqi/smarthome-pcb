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
    uint32_t timestamp_ms;
    uint32_t valid_mask;
    int16_t temperature_centi_c;
    uint16_t humidity_centi_pct;
    uint32_t pressure_pa;
    uint16_t light_raw;
    uint16_t mq2_raw;
    uint16_t mq7_raw;
    uint16_t mq135_raw;
} bsp_sensor_snapshot_t;

bsp_status_t BSP_Sensors_Init(void);
bsp_status_t BSP_Sensors_Read(bsp_sensor_snapshot_t *snapshot);

#endif
