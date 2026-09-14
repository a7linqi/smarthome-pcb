#include "app_model.h"

#include <string.h>

static SemaphoreHandle_t app_mutex;
static AppSnapshot app_snapshot;

QueueHandle_t g_app_command_queue;
QueueHandle_t g_app_ack_queue;

bool AppModel_Init(void)
{
    memset(&app_snapshot, 0, sizeof(app_snapshot));
    app_snapshot.mode = APP_MODE_MANUAL;
    app_snapshot.temperature_high = 30U;
    app_snapshot.soil_low = 40U;
    app_snapshot.sensor.dht11_valid = false;

    app_mutex = xSemaphoreCreateMutex();
    g_app_command_queue = xQueueCreate(8U, sizeof(AppCommand));
    g_app_ack_queue = xQueueCreate(4U, sizeof(AppControlAck));

    return (app_mutex != NULL) && (g_app_command_queue != NULL) &&
           (g_app_ack_queue != NULL);
}

void AppModel_GetSnapshot(AppSnapshot *snapshot)
{
    if ((snapshot == NULL) || (app_mutex == NULL)) {
        return;
    }

    if (xSemaphoreTake(app_mutex, portMAX_DELAY) == pdTRUE) {
        *snapshot = app_snapshot;
        xSemaphoreGive(app_mutex);
    }
}

void AppModel_UpdateSensor(const SensorSnapshot *sensor)
{
    if ((sensor == NULL) || (app_mutex == NULL)) {
        return;
    }

    if (xSemaphoreTake(app_mutex, portMAX_DELAY) == pdTRUE) {
        app_snapshot.sensor = *sensor;
        xSemaphoreGive(app_mutex);
    }
}

void AppModel_UpdateRuntime(AppMode mode, AppAlarm alarm, bool pump_on)
{
    if (xSemaphoreTake(app_mutex, portMAX_DELAY) == pdTRUE) {
        app_snapshot.mode = mode;
        app_snapshot.alarm = alarm;
        app_snapshot.pump_on = pump_on;
        xSemaphoreGive(app_mutex);
    }
}

void AppModel_SetMqttOnline(bool online)
{
    if (xSemaphoreTake(app_mutex, portMAX_DELAY) == pdTRUE) {
        app_snapshot.mqtt_online = online;
        xSemaphoreGive(app_mutex);
    }
}

void AppModel_SetUi(uint8_t page, uint8_t selection)
{
    if (xSemaphoreTake(app_mutex, portMAX_DELAY) == pdTRUE) {
        app_snapshot.ui_page = page;
        app_snapshot.ui_selection = selection;
        xSemaphoreGive(app_mutex);
    }
}

void AppModel_SetSettings(uint16_t temperature_high, uint16_t soil_low)
{
    if (xSemaphoreTake(app_mutex, portMAX_DELAY) == pdTRUE) {
        app_snapshot.temperature_high = temperature_high;
        app_snapshot.soil_low = soil_low;
        xSemaphoreGive(app_mutex);
    }
}
