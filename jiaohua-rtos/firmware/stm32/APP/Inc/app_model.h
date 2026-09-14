#ifndef APP_MODEL_H
#define APP_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef enum {
    APP_MODE_AUTO = 0,
    APP_MODE_MANUAL = 1
} AppMode;

typedef enum {
    APP_ALARM_NONE = 0,
    APP_ALARM_SOIL_DRY,
    APP_ALARM_TEMPERATURE_HIGH,
    APP_ALARM_DHT11_FAULT,
    APP_ALARM_PUMP_TIMEOUT
} AppAlarm;

typedef struct {
    uint16_t temperature;
    uint16_t humidity;
    uint16_t soil_percent;
    uint16_t soil_adc;
    bool dht11_valid;
} SensorSnapshot;

typedef struct {
    SensorSnapshot sensor;
    AppMode mode;
    AppAlarm alarm;
    uint16_t temperature_high;
    uint16_t soil_low;
    bool pump_on;
    bool network_enabled;
    bool mqtt_online;
    uint8_t ui_page;
    uint8_t ui_selection;
} AppSnapshot;

typedef enum {
    APP_CMD_SET_AUTO = 0,
    APP_CMD_SET_MANUAL,
    APP_CMD_PUMP_ON,
    APP_CMD_PUMP_OFF,
    APP_CMD_SET_TEMPERATURE_HIGH,
    APP_CMD_SET_SOIL_LOW,
    APP_CMD_UI_NEXT_PAGE,
    APP_CMD_UI_NEXT_ITEM,
    APP_CMD_UI_INCREASE,
    APP_CMD_UI_DECREASE
} AppCommandType;

typedef struct {
    AppCommandType type;
    uint16_t value;
    uint8_t sequence;
    bool requires_ack;
} AppCommand;

typedef struct {
    AppCommandType type;
    uint16_t value;
    uint8_t sequence;
    uint8_t status;
} AppControlAck;

extern QueueHandle_t g_app_command_queue;
extern QueueHandle_t g_app_ack_queue;

bool AppModel_Init(void);
void AppModel_GetSnapshot(AppSnapshot *snapshot);
void AppModel_UpdateSensor(const SensorSnapshot *sensor);
void AppModel_UpdateRuntime(AppMode mode, AppAlarm alarm, bool pump_on);
void AppModel_SetNetworkEnabled(bool enabled);
void AppModel_SetMqttOnline(bool online);
void AppModel_SetUi(uint8_t page, uint8_t selection);
void AppModel_SetSettings(uint16_t temperature_high, uint16_t soil_low);

#endif
