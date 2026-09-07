#include "uart_task.h"

#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "bsp_protocol.h"
#include "bsp_sensors.h"
#include "bsp_uart_bridge.h"

extern QueueHandle_t uartSensorQueue;
extern QueueHandle_t remoteControlQueue;
extern QueueHandle_t controlAckQueue;
extern QueueHandle_t deviceStateQueue;

static uint8_t tx_sequence;
static TickType_t last_heartbeat_tick;
static uint8_t heartbeat_received;

bool Uart_IsEspOnline(void)
{
    if (heartbeat_received == 0U) {
        return false;
    }
    return (xTaskGetTickCount() - last_heartbeat_tick) < pdMS_TO_TICKS(5000);
}

static void put_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
}

static void put_u32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
    dst[2] = (uint8_t)(value >> 16);
    dst[3] = (uint8_t)(value >> 24);
}

static void send_frame(uint8_t type, uint8_t sequence,
                       const uint8_t *payload, uint16_t length)
{
    uint8_t frame[BSP_PROTOCOL_MAX_FRAME];
    size_t frame_length = BSP_Protocol_Encode(type, sequence, payload, length,
                                               frame, sizeof(frame));
    if (frame_length != 0U) {
        (void)BSP_UartBridge_Write(frame, frame_length, 20U);
    }
}

static void send_sensor_report(const bsp_sensor_snapshot_t *sensor)
{
    uint8_t payload[24];

    put_u32_le(payload + 0, sensor->timestamp_ms);
    put_u32_le(payload + 4, sensor->valid_mask);
    put_u16_le(payload + 8, (uint16_t)sensor->temperature_centi_c);
    put_u16_le(payload + 10, sensor->humidity_centi_pct);
    put_u32_le(payload + 12, sensor->pressure_pa);
    put_u16_le(payload + 16, sensor->light_raw);
    put_u16_le(payload + 18, sensor->mq2_raw);
    put_u16_le(payload + 20, sensor->mq7_raw);
    put_u16_le(payload + 22, sensor->mq135_raw);
    send_frame(BSP_MSG_SENSOR_REPORT, tx_sequence++, payload, sizeof(payload));
}

static void send_device_state(const app_device_state_t *state)
{
    uint8_t payload[3] = {state->mode, state->light_on, state->door_open};
    send_frame(BSP_MSG_DEVICE_STATE, tx_sequence++, payload, sizeof(payload));
}

static void send_control_ack(const app_control_ack_t *ack)
{
    uint8_t payload[3] = {ack->status, ack->device, ack->value};
    /* ACK 沿用命令 sequence，ESP8266 才能将请求与结果对应。 */
    send_frame(BSP_MSG_CONTROL_ACK, ack->sequence, payload, sizeof(payload));
}

static void handle_frame(const bsp_protocol_frame_t *frame)
{
    if (frame->type == BSP_MSG_HEARTBEAT) {
        last_heartbeat_tick = xTaskGetTickCount();
        heartbeat_received = 1U;
        /* ESP8266 主动发心跳；STM32 原样回送 payload 和 sequence。 */
        send_frame(BSP_MSG_HEARTBEAT, frame->sequence,
                   frame->payload, frame->length);
    } else if (frame->type == BSP_MSG_CONTROL_COMMAND && frame->length == 2U) {
        app_control_command_t command;
        command.sequence = frame->sequence;
        command.device = frame->payload[0];
        command.value = frame->payload[1];
        if (xQueueSend(remoteControlQueue, &command, 0) != pdTRUE) {
            app_control_ack_t ack = {
                frame->sequence, APP_ACK_BUSY, command.device, command.value
            };
            send_control_ack(&ack);
        }
    }
}

void Uart_Task(void *params)
{
    bsp_protocol_parser_t parser;
    bsp_protocol_frame_t received_frame;
    bsp_sensor_snapshot_t sensor;
    app_control_ack_t ack;
    app_device_state_t state;
    uint8_t rx_data[32];

    (void)params;
    BSP_Protocol_ParserInit(&parser);

    for (;;) {
        size_t count = BSP_UartBridge_Read(rx_data, sizeof(rx_data));
        for (size_t i = 0; i < count; ++i) {
            if (BSP_Protocol_Feed(&parser, rx_data[i], &received_frame) == BSP_OK) {
                handle_frame(&received_frame);
            }
        }

        /* 长度为 1 的队列保存最新状态；一次循环将已有数据全部取走。 */
        while (xQueueReceive(uartSensorQueue, &sensor, 0) == pdTRUE) {
            send_sensor_report(&sensor);
        }
        while (xQueueReceive(deviceStateQueue, &state, 0) == pdTRUE) {
            send_device_state(&state);
        }
        while (xQueueReceive(controlAckQueue, &ack, 0) == pdTRUE) {
            send_control_ack(&ack);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
