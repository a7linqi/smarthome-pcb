#include "app_main.h"

#include "FreeRTOS.h"
#include "task.h"

#include "app_model.h"
#include "comm_task.h"
#include "control_task.h"
#include "display_task.h"
#include "esp8266.h"
#include "key_task.h"
#include "sensor_task.h"
#include "stmflash.h"

static void FatalError(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void AppMain_Start(bool network_enabled)
{
    uint16_t temperature_high;
    uint16_t soil_low;

    if (!AppModel_Init()) {
        FatalError();
    }

    STMFLASH_Read(FLASH_SAVE_ADDR, &temperature_high, 1U);
    STMFLASH_Read(FLASH_SAVE_ADDR + 4U, &soil_low, 1U);

    if ((temperature_high < 10U) || (temperature_high > 60U)) {
        temperature_high = 30U;
    }
    if ((soil_low < 5U) || (soil_low > 95U)) {
        soil_low = 40U;
    }
    AppModel_SetSettings(temperature_high, soil_low);

    if (xTaskCreate(ControlTask, "control", 256U, NULL, 4U, NULL) != pdPASS) {
        FatalError();
    }
    if (xTaskCreate(SensorTask, "sensor", 256U, NULL, 3U, NULL) != pdPASS) {
        FatalError();
    }
    if (network_enabled) {
        if (xTaskCreate(CommTask, "comm", 384U, NULL, 3U, NULL) != pdPASS) {
            FatalError();
        }
    } else {
        ESP8266_Disable();
        AppModel_SetMqttOnline(false);
    }
    if (xTaskCreate(KeyTask, "key", 192U, NULL, 2U, NULL) != pdPASS) {
        FatalError();
    }
    if (xTaskCreate(DisplayTask, "display", 256U, NULL, 1U, NULL) != pdPASS) {
        FatalError();
    }

    vTaskStartScheduler();
    FatalError();
}

void vApplicationMallocFailedHook(void)
{
    FatalError();
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    FatalError();
}
