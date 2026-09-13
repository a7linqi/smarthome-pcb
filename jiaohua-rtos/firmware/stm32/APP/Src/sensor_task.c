#include "sensor_task.h"

#include "FreeRTOS.h"
#include "task.h"

#include "adc.h"
#include "app_model.h"
#include "dht11.h"

/* Provisional values from the first dry/wet-tissue test.
 * Replace them with measurements taken in the real flowerpot. */
#define SOIL_ADC_DRY       1843U
#define SOIL_ADC_WET        410U
#define SOIL_SAMPLE_MS      500U
#define DHT11_SAMPLE_MS    2000U

static uint16_t SoilAdcToPercent(uint16_t adc)
{
    if (adc >= SOIL_ADC_DRY) {
        return 0U;
    }

    if (adc <= SOIL_ADC_WET) {
        return 100U;
    }

    return (uint16_t)(((uint32_t)(SOIL_ADC_DRY - adc) * 100U) /
                      (SOIL_ADC_DRY - SOIL_ADC_WET));
}

void SensorTask(void *argument)
{
    SensorSnapshot sensor = {0};
    DHT11_Data_TypeDef dht11 = {0};
    TickType_t last_wake = xTaskGetTickCount();
    TickType_t last_dht11 = 0U;

    (void)argument;

    for (;;) {
        const TickType_t now = xTaskGetTickCount();

        sensor.soil_adc = Get_Adc_Average(5U, 10U);
        sensor.soil_percent = SoilAdcToPercent(sensor.soil_adc);

        if ((now - last_dht11) >= pdMS_TO_TICKS(DHT11_SAMPLE_MS)) {
            /* DHT11 uses microsecond pulse widths. Keep the transaction from
             * being split by a task switch; interrupts remain enabled. */
            vTaskSuspendAll();
            sensor.dht11_valid = (Read_DHT11(&dht11) == SUCCESS);
            (void)xTaskResumeAll();
            if (sensor.dht11_valid) {
                sensor.temperature = dht11.temp_int;
                sensor.humidity = dht11.humi_int;
            }
            last_dht11 = now;
        }

        AppModel_UpdateSensor(&sensor);
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SOIL_SAMPLE_MS));
    }
}
