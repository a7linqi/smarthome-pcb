#include "bsp_sensors.h"
#include "bsp_adc.h"
#include "bsp_bmp280.h"
#include "bsp_dht11.h"
#include "stm32f1xx_hal.h"
#include <string.h>

static uint8_t bmp_ready;

bsp_status_t BSP_Sensors_Init(void)
{
    bmp_ready = BSP_BMP280_Init() == BSP_OK;
    return bmp_ready ? BSP_OK : BSP_ERROR;
}

bsp_status_t BSP_Sensors_Read(bsp_sensor_snapshot_t *s)
{
    bsp_dht11_data_t dht;
    bsp_bmp280_data_t bmp;
    if (!s) return BSP_INVALID_ARG;
    memset(s, 0, sizeof(*s));
    s->timestamp_ms = HAL_GetTick();
    s->light_raw = (uint16_t)(4095U - BSP_ADC_GetFiltered(BSP_ADC_LIGHT));
    s->mq7_raw = BSP_ADC_GetFiltered(BSP_ADC_MQ7);
    s->mq135_raw = (uint16_t)(4095U - BSP_ADC_GetFiltered(BSP_ADC_MQ135));
    s->mq2_raw = BSP_ADC_GetFiltered(BSP_ADC_MQ2);
    s->valid_mask |= BSP_SENSOR_VALID_ADC;
    if (BSP_DHT11_Read(&dht) == BSP_OK) {
        s->temperature_centi_c = (int16_t)dht.temperature_c * 100;
        s->humidity_centi_pct = (uint16_t)dht.humidity_pct * 100U;
        s->valid_mask |= BSP_SENSOR_VALID_DHT11;
    }
    if (bmp_ready && BSP_BMP280_Read(&bmp) == BSP_OK) {
        s->pressure_pa = bmp.pressure_pa;
        if (!(s->valid_mask & BSP_SENSOR_VALID_DHT11)) s->temperature_centi_c = (int16_t)bmp.temperature_centi_c;
        s->valid_mask |= BSP_SENSOR_VALID_BMP280;
    }
    return s->valid_mask ? BSP_OK : BSP_ERROR;
}
