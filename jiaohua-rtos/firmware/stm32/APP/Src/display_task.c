#include "display_task.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "OLED.h"
#include "app_model.h"

#define DISPLAY_PERIOD_MS  200U

static void ShowDataPage(const AppSnapshot *snapshot)
{
    char line[24];

    if (snapshot->sensor.dht11_valid) {
        sprintf(line, "Temp:%uC %s ", snapshot->sensor.temperature,
                snapshot->mode == APP_MODE_AUTO ? "AUTO" : "MAN ");
        OLED_ShowText(0U, 0U, (u8 *)line, 0U);

        sprintf(line, "Air Humi:%u%%   ", snapshot->sensor.humidity);
        OLED_ShowText(0U, 2U, (u8 *)line, 0U);
    } else {
        OLED_ShowText(0U, 0U, (u8 *)"DHT11:ERROR     ", 0U);
        OLED_ShowText(0U, 2U, (u8 *)"Air Humi:--     ", 0U);
    }

    sprintf(line, "Soil:%u%% ADC:%u ", snapshot->sensor.soil_percent,
            snapshot->sensor.soil_adc);
    OLED_ShowText(0U, 4U, (u8 *)line, 0U);

    sprintf(line, "Pump:%s MQTT:%s ", snapshot->pump_on ? "ON " : "OFF",
            snapshot->mqtt_online ? "OK" : "--");
    OLED_ShowText(0U, 6U, (u8 *)line, 0U);
}

static void ShowControlPage(const AppSnapshot *snapshot)
{
    char line[24];
    const char marker0 = snapshot->ui_selection == 0U ? '>' : ' ';
    const char marker1 = snapshot->ui_selection == 1U ? '>' : ' ';
    const char marker2 = snapshot->ui_selection == 2U ? '>' : ' ';
    const char marker3 = snapshot->ui_selection == 3U ? '>' : ' ';

    sprintf(line, "%cMode:%s       ", marker0,
            snapshot->mode == APP_MODE_AUTO ? "AUTO" : "MAN ");
    OLED_ShowText(0U, 0U, (u8 *)line, 0U);

    sprintf(line, "%cPump:%s       ", marker1,
            snapshot->pump_on ? "ON " : "OFF");
    OLED_ShowText(0U, 2U, (u8 *)line, 0U);

    sprintf(line, "%cTemp high:%u  ", marker2, snapshot->temperature_high);
    OLED_ShowText(0U, 4U, (u8 *)line, 0U);

    sprintf(line, "%cSoil low:%u   ", marker3, snapshot->soil_low);
    OLED_ShowText(0U, 6U, (u8 *)line, 0U);
}

void DisplayTask(void *argument)
{
    AppSnapshot snapshot;
    uint8_t last_page = 0xFFU;
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;

    for (;;) {
        AppModel_GetSnapshot(&snapshot);

        if (snapshot.ui_page != last_page) {
            OLED_Clear(0U);
            last_page = snapshot.ui_page;
        }

        if (snapshot.ui_page == 0U) {
            ShowDataPage(&snapshot);
        } else {
            ShowControlPage(&snapshot);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
    }
}
