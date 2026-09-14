#include "comm_task.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "app_model.h"
#include "esp8266.h"

#define COMM_POLL_MS       20U
#define TELEMETRY_MS     2000U

extern unsigned char esp8266_buf[buf_len];

/* Used by the legacy ESP8266 initialization driver. */
char Flagout = 1;

static void QueueCommand(AppCommandType type, uint16_t value)
{
    AppCommand command;

    command.type = type;
    command.value = value;
    (void)xQueueSend(g_app_command_queue, &command, 0U);
}

static void ParseRemoteCommand(const char *text)
{
    const char *value_text;
    int value;

    if (strstr(text, "+MQTT:ONLINE") != NULL) {
        AppModel_SetMqttOnline(true);
    } else if (strstr(text, "+MQTT:OFFLINE") != NULL) {
        AppModel_SetMqttOnline(false);
    } else if (strstr(text, "SET_MODE:ZD") != NULL) {
        QueueCommand(APP_CMD_SET_AUTO, 0U);
    } else if (strstr(text, "SET_MODE:SD") != NULL) {
        QueueCommand(APP_CMD_SET_MANUAL, 0U);
    } else if (strstr(text, "Water_pump_ON") != NULL) {
        QueueCommand(APP_CMD_PUMP_ON, 0U);
    } else if (strstr(text, "Water_pump_OFF") != NULL) {
        QueueCommand(APP_CMD_PUMP_OFF, 0U);
    } else if ((value_text = strstr(text, "SET_T_H:")) != NULL) {
        if (sscanf(value_text + 8, "%d", &value) == 1) {
            QueueCommand(APP_CMD_SET_TEMPERATURE_HIGH, (uint16_t)value);
        }
    } else if ((value_text = strstr(text, "SET_H_L:")) != NULL) {
        if (sscanf(value_text + 8, "%d", &value) == 1) {
            QueueCommand(APP_CMD_SET_SOIL_LOW, (uint16_t)value);
        }
    }
}

static void PublishTelemetry(const AppSnapshot *snapshot)
{
    char message[160];

    sprintf(message,
            "cmd=2&uid=%s&topic=data&msg=#%s#%u#%u#%u#%u#%u#sensordata#\r\n",
            BEMFA_ID,
            snapshot->mode == APP_MODE_AUTO ? "ZD" : "SD",
            snapshot->sensor.temperature,
            snapshot->sensor.humidity,
            snapshot->sensor.soil_percent,
            snapshot->pump_on ? 1U : 0U,
            snapshot->alarm != APP_ALARM_NONE ? 1U : 0U);

    ESP8266_SendData((unsigned char *)message);
}

void CommTask(void *argument)
{
    TickType_t last_publish;
    AppSnapshot snapshot;

    (void)argument;

    ESP8266_Init(115200U);
    /* ESP8266_Init returns only after Wi-Fi, MQTT and subscription succeed. */
    AppModel_SetMqttOnline(true);
    last_publish = xTaskGetTickCount();

    for (;;) {
        if (ESP8266_WaitRecive() == REV_OK) {
            ParseRemoteCommand((const char *)esp8266_buf);
            ESP8266_Clear();
        }

        if ((xTaskGetTickCount() - last_publish) >=
            pdMS_TO_TICKS(TELEMETRY_MS)) {
            AppModel_GetSnapshot(&snapshot);
            PublishTelemetry(&snapshot);
            last_publish = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(COMM_POLL_MS));
    }
}
