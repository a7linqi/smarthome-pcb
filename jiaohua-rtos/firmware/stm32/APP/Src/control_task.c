#include "control_task.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "app_model.h"
#include "led.h"
#include "stmflash.h"

#define CONTROL_PERIOD_MS       100U
#define SOIL_HYSTERESIS          10U
#define PUMP_MAX_RUN_MS       30000U
#define SENSOR_STARTUP_GRACE_MS 3000U
#define TEMPERATURE_MIN          10U
#define TEMPERATURE_MAX          60U
#define SOIL_THRESHOLD_MIN        5U
#define SOIL_THRESHOLD_MAX       95U

static void SaveSettings(uint16_t temperature_high, uint16_t soil_low)
{
    STMFLASH_Write(FLASH_SAVE_ADDR, &temperature_high, 1U);
    STMFLASH_Write(FLASH_SAVE_ADDR + 4U, &soil_low, 1U);
}

static void SendControlAck(const AppCommand *command, bool accepted,
                           uint16_t actual_value)
{
    AppControlAck ack;

    if (!command->requires_ack) {
        return;
    }

    ack.type = command->type;
    ack.value = actual_value;
    ack.sequence = command->sequence;
    ack.status = accepted ? 0U : 1U;
    (void)xQueueSend(g_app_ack_queue, &ack, 0U);
}

void ControlTask(void *argument)
{
    AppMode mode = APP_MODE_MANUAL;
    AppAlarm alarm = APP_ALARM_NONE;
    bool pump_on = false;
    bool pump_timer_running = false;
    bool pump_timeout_latched = false;
    TickType_t pump_started_at = 0U;
    AppSnapshot snapshot;
    AppCommand command;
    TickType_t last_wake = xTaskGetTickCount();

    (void)argument;

    AppModel_GetSnapshot(&snapshot);

    for (;;) {
        bool settings_changed = false;

        while (xQueueReceive(g_app_command_queue, &command, 0U) == pdTRUE) {
            bool accepted = true;
            uint16_t actual_value = 0U;

            switch (command.type) {
            case APP_CMD_SET_AUTO:
                mode = APP_MODE_AUTO;
                pump_timeout_latched = false;
                actual_value = (uint16_t)mode;
                break;

            case APP_CMD_SET_MANUAL:
                mode = APP_MODE_MANUAL;
                pump_on = false;
                pump_timeout_latched = false;
                actual_value = (uint16_t)mode;
                break;

            case APP_CMD_PUMP_ON:
                if (mode == APP_MODE_MANUAL) {
                    pump_on = true;
                } else {
                    accepted = false;
                }
                actual_value = pump_on ? 1U : 0U;
                break;

            case APP_CMD_PUMP_OFF:
                if (mode == APP_MODE_MANUAL) {
                    pump_on = false;
                } else {
                    accepted = false;
                }
                actual_value = pump_on ? 1U : 0U;
                break;

            case APP_CMD_SET_TEMPERATURE_HIGH:
                if ((command.value >= TEMPERATURE_MIN) &&
                    (command.value <= TEMPERATURE_MAX)) {
                    snapshot.temperature_high = command.value;
                    settings_changed = true;
                } else {
                    accepted = false;
                }
                actual_value = snapshot.temperature_high;
                break;

            case APP_CMD_SET_SOIL_LOW:
                if ((command.value >= SOIL_THRESHOLD_MIN) &&
                    (command.value <= SOIL_THRESHOLD_MAX)) {
                    snapshot.soil_low = command.value;
                    settings_changed = true;
                } else {
                    accepted = false;
                }
                actual_value = snapshot.soil_low;
                break;

            default:
                accepted = false;
                break;
            }

            SendControlAck(&command, accepted, actual_value);
        }

        if (settings_changed) {
            AppModel_SetSettings(snapshot.temperature_high, snapshot.soil_low);
            SaveSettings(snapshot.temperature_high, snapshot.soil_low);
        }

        AppModel_GetSnapshot(&snapshot);

        if (mode == APP_MODE_AUTO) {
            const uint16_t stop_threshold =
                (snapshot.soil_low <= (100U - SOIL_HYSTERESIS))
                    ? (uint16_t)(snapshot.soil_low + SOIL_HYSTERESIS)
                    : 100U;

            if (snapshot.sensor.soil_percent >= stop_threshold) {
                pump_on = false;
                pump_timeout_latched = false;
            } else if ((snapshot.sensor.soil_percent <= snapshot.soil_low) &&
                       !pump_timeout_latched) {
                pump_on = true;
            }
        }

        if (pump_on && !pump_timer_running) {
            pump_started_at = xTaskGetTickCount();
            pump_timer_running = true;
        } else if (!pump_on) {
            pump_timer_running = false;
        }

        if (pump_on &&
            ((xTaskGetTickCount() - pump_started_at) >=
             pdMS_TO_TICKS(PUMP_MAX_RUN_MS))) {
            pump_on = false;
            pump_timeout_latched = true;
        }

        if (pump_timeout_latched) {
            alarm = APP_ALARM_PUMP_TIMEOUT;
        } else if ((xTaskGetTickCount() >=
                    pdMS_TO_TICKS(SENSOR_STARTUP_GRACE_MS)) &&
                   !snapshot.sensor.dht11_valid) {
            alarm = APP_ALARM_DHT11_FAULT;
        } else if (snapshot.sensor.temperature >= snapshot.temperature_high) {
            alarm = APP_ALARM_TEMPERATURE_HIGH;
        } else if (snapshot.sensor.soil_percent <= snapshot.soil_low) {
            alarm = APP_ALARM_SOIL_DRY;
        } else {
            alarm = APP_ALARM_NONE;
        }

        Water_pump = pump_on ? ON : OFF;
        BEEP = (alarm == APP_ALARM_NONE) ? OFF : ON;
        AppModel_UpdateRuntime(mode, alarm, pump_on);

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    }
}
