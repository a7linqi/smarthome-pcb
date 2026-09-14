#include "display_task.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "OLED.h"
#include "app_model.h"

#define DISPLAY_PERIOD_MS  200U

/* OLED_ShowText expects two-byte GBK codes for Chinese glyph lookup. Keeping
 * the byte sequences explicit makes the display independent of source-file
 * encoding and Keil's local code-page setting. */
static const char TEXT_AUTO[] = "\xD7\xD4\xB6\xAF";
static const char TEXT_MANUAL[] = "\xCA\xD6\xB6\xAF";

static void ShowNetworkPage(uint8_t progress)
{
    char line[20];

    OLED_ShowText(0U, 2U,
                  (u8 *)"\xD5\xFD\xD4\xDA\xC1\xAC\xBD\xD3 WIFI...",
                  0U);
    sprintf(line, "\xBD\xF8\xB6\xC8:%u%%   ", progress);
    OLED_ShowText(24U, 6U, (u8 *)line, 0U);
}

static void ShowDataPage(const AppSnapshot *snapshot)
{
    char line[32];
    const char *mode = snapshot->mode == APP_MODE_AUTO
                           ? TEXT_AUTO
                           : TEXT_MANUAL;
    const char *network_status = !snapshot->network_enabled
                                     ? "NO"
                                     : (snapshot->mqtt_online ? "OK" : "--");

    if (snapshot->sensor.dht11_valid) {
        sprintf(line, "\xCE\xC2\xB6\xC8:%u\xA1\xE6 %s ",
                snapshot->sensor.temperature, mode);
        OLED_ShowText(0U, 0U, (u8 *)line, 0U);

        sprintf(line, "\xCA\xAA\xB6\xC8:%u%%       ",
                snapshot->sensor.humidity);
        OLED_ShowText(0U, 2U, (u8 *)line, 0U);
    } else {
        OLED_ShowText(0U, 0U,
                      (u8 *)"\xCE\xC2\xB6\xC8:--         ", 0U);
        OLED_ShowText(0U, 2U,
                      (u8 *)"\xCA\xAA\xB6\xC8:--         ", 0U);
    }

    sprintf(line, "\xCD\xC1\xC8\xC0\xCA\xAA\xB6\xC8:%u%%   ",
            snapshot->sensor.soil_percent);
    OLED_ShowText(0U, 4U, (u8 *)line, 0U);

    sprintf(line, "\xCB\xAE\xB1\xC3:%s \xC1\xAA\xCD\xF8:%s",
            snapshot->pump_on ? "ON " : "OFF",
            network_status);
    OLED_ShowText(0U, 6U, (u8 *)line, 0U);
}

static void ShowControlPage(const AppSnapshot *snapshot)
{
    char line[32];
    const char marker0 = snapshot->ui_selection == 0U ? '>' : ' ';
    const char marker1 = snapshot->ui_selection == 1U ? '>' : ' ';
    const char marker2 = snapshot->ui_selection == 2U ? '>' : ' ';
    const char marker3 = snapshot->ui_selection == 3U ? '>' : ' ';

    sprintf(line, "%c\xC4\xA3\xCA\xBD:%s      ", marker0,
            snapshot->mode == APP_MODE_AUTO ? TEXT_AUTO : TEXT_MANUAL);
    OLED_ShowText(0U, 0U, (u8 *)line, 0U);

    sprintf(line, "%c\xCB\xAE\xB1\xC3:%s       ", marker1,
            snapshot->pump_on ? "ON " : "OFF");
    OLED_ShowText(0U, 2U, (u8 *)line, 0U);

    sprintf(line, "%c\xCE\xC2\xB6\xC8\xC9\xCF\xCF\xDE:%u  ",
            marker2, snapshot->temperature_high);
    OLED_ShowText(0U, 4U, (u8 *)line, 0U);

    sprintf(line, "%c\xCA\xAA\xB6\xC8\xCF\xC2\xCF\xDE:%u   ",
            marker3, snapshot->soil_low);
    OLED_ShowText(0U, 6U, (u8 *)line, 0U);
}

void DisplayTask(void *argument)
{
    AppSnapshot snapshot;
    uint8_t last_view = 0xFFU;
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;

    for (;;) {
        uint8_t view;

        AppModel_GetSnapshot(&snapshot);
        view = (snapshot.network_enabled &&
                (snapshot.network_progress < 100U))
                   ? 2U
                   : snapshot.ui_page;

        if (view != last_view) {
            OLED_Clear(0U);
            last_view = view;
        }

        if (view == 2U) {
            ShowNetworkPage(snapshot.network_progress);
        } else if (view == 0U) {
            ShowDataPage(&snapshot);
        } else {
            ShowControlPage(&snapshot);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
    }
}
