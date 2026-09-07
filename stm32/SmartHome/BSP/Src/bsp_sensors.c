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
    /*
     * valid_mask 位掩码机制：
     *   每个传感器占一个 bit，读成功就用 |= 置1，上层用 & 检查单个 bit。
     *   例如：初始 0b000
     *     |= ADC    → 0b100   （bit2 置1）
     *     |= DHT11  → 0b101   （bit0 置1）
     *     |= BMP280 → 0b111   （bit1 置1）
     *   检查时：valid_mask & BSP_SENSOR_VALID_DHT11 非0即为成功
     */
    /* 读取 ADC 通道：光敏、MQ-7、MQ-135、MQ-2 */
    s->light_raw = (uint16_t)(4095U - BSP_ADC_GetFiltered(BSP_ADC_LIGHT));
    s->mq7_raw   = BSP_ADC_GetFiltered(BSP_ADC_MQ7);
    s->mq135_raw = (uint16_t)(4095U - BSP_ADC_GetFiltered(BSP_ADC_MQ135));
    s->mq2_raw   = BSP_ADC_GetFiltered(BSP_ADC_MQ2);
    s->valid_mask |= BSP_SENSOR_VALID_ADC;   /* bit2 置1，ADC 通道读取成功 */
    /* 读取 DHT11 温湿度 */
    if (BSP_DHT11_Read(&dht) == BSP_OK) {
        s->temperature_centi_c = (int16_t)dht.temperature_c * 100;
        s->humidity_centi_pct  = (uint16_t)dht.humidity_pct * 100U;
        s->valid_mask |= BSP_SENSOR_VALID_DHT11;   /* bit0 置1，DHT11 读取成功 */
    }
    /* 读取 BMP280 气压，温度作为 DHT11 的备用 */
    if (bmp_ready && BSP_BMP280_Read(&bmp) == BSP_OK) {
        s->pressure_pa = bmp.pressure_pa;
        /* 如果 DHT11 失败(bit0 为0)，用 BMP280 温度兜底 */
        if (!(s->valid_mask & BSP_SENSOR_VALID_DHT11))
            s->temperature_centi_c = (int16_t)bmp.temperature_centi_c;
        s->valid_mask |= BSP_SENSOR_VALID_BMP280;   /* bit1 置1，BMP280 读取成功 */
    }
    /* 至少一个传感器成功就返回 OK，全失败返回 ERROR */
    return s->valid_mask ? BSP_OK : BSP_ERROR;
}
