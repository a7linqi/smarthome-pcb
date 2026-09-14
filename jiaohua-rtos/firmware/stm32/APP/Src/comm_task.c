#include "comm_task.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "app_model.h"
#include "app_protocol.h"
#include "esp8266.h"

#define COMM_POLL_MS       20U
#define TELEMETRY_MS     2000U
#define HEARTBEAT_MS     5000U
#define TELEMETRY_LENGTH   11U

static uint8_t tx_sequence;

static void SendFrame(uint8_t type, uint8_t sequence,
                      const uint8_t *payload, uint16_t length)
{
    uint8_t output[APP_PROTOCOL_MAX_FRAME];
    const size_t count = AppProtocol_Encode(type, sequence, payload, length,
                                            output, sizeof(output));

    if (count > 0U) {
        ESP8266_SendData(output, (uint16_t)count);
    }
}

static void QueueRemoteCommand(const AppProtocolFrame *frame)
{
    AppCommand command;
    const uint8_t wire_command = frame->payload[0];

    if ((frame->length != 3U) || (wire_command < 1U) ||
        (wire_command > 6U)) {
        return;
    }

    command.type = (AppCommandType)(wire_command - 1U);
    command.value = AppProtocol_GetU16(frame->payload + 1U);
    command.sequence = frame->sequence;
    command.requires_ack = true;
    (void)xQueueSend(g_app_command_queue, &command, 0U);
}

static void HandleFrame(const AppProtocolFrame *frame)
{
    if (frame->type == APP_MSG_CONTROL_COMMAND) {
        QueueRemoteCommand(frame);
    } else if ((frame->type == APP_MSG_MQTT_STATUS) &&
               (frame->length == 1U)) {
        AppModel_SetMqttOnline(frame->payload[0] != 0U);
    }
}

static void PublishTelemetry(const AppSnapshot *snapshot)
{
    uint8_t payload[TELEMETRY_LENGTH];

    AppProtocol_PutU16(payload + 0U, snapshot->sensor.temperature);
    AppProtocol_PutU16(payload + 2U, snapshot->sensor.humidity);
    AppProtocol_PutU16(payload + 4U, snapshot->sensor.soil_percent);
    AppProtocol_PutU16(payload + 6U, snapshot->sensor.soil_adc);
    payload[8] = (uint8_t)snapshot->mode;
    payload[9] = snapshot->pump_on ? 1U : 0U;
    payload[10] = (uint8_t)snapshot->alarm;

    SendFrame(APP_MSG_TELEMETRY, tx_sequence++, payload, sizeof(payload));
}

static void PublishControlAck(const AppControlAck *ack)
{
    uint8_t payload[4];

    payload[0] = (uint8_t)ack->type + 1U;
    payload[1] = ack->status;
    AppProtocol_PutU16(payload + 2U, ack->value);
    SendFrame(APP_MSG_CONTROL_ACK, ack->sequence, payload, sizeof(payload));
}

void CommTask(void *argument)
{
    TickType_t last_publish;
    TickType_t last_heartbeat;
    AppSnapshot snapshot;
    AppControlAck ack;
    AppProtocolParser parser;
    AppProtocolFrame frame;
    uint8_t received[buf_len];

    (void)argument;

    ESP8266_Init(115200U);
    AppProtocol_ParserInit(&parser);
    AppModel_SetMqttOnline(false);
    last_publish = xTaskGetTickCount();
    last_heartbeat = last_publish;

    for (;;) {
        uint16_t index;
        const uint16_t count = ESP8266_ReadReceived(received,
                                                    sizeof(received));

        for (index = 0U; index < count; ++index) {
            if (AppProtocol_Feed(&parser, received[index], &frame) ==
                APP_PARSE_FRAME) {
                HandleFrame(&frame);
            }
        }

        while (xQueueReceive(g_app_ack_queue, &ack, 0U) == pdTRUE) {
            PublishControlAck(&ack);
        }

        if ((xTaskGetTickCount() - last_publish) >=
            pdMS_TO_TICKS(TELEMETRY_MS)) {
            AppModel_GetSnapshot(&snapshot);
            PublishTelemetry(&snapshot);
            last_publish = xTaskGetTickCount();
        }

        if ((xTaskGetTickCount() - last_heartbeat) >=
            pdMS_TO_TICKS(HEARTBEAT_MS)) {
            SendFrame(APP_MSG_HEARTBEAT, tx_sequence++, NULL, 0U);
            last_heartbeat = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(COMM_POLL_MS));
    }
}
