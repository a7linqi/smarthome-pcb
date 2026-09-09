#include "uart_task.h"

#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "bsp_protocol.h"
#include "bsp_sensors.h"
#include "bsp_uart_bridge.h"

/* ---- 外部队列：由其他任务（sensor_task、control_task）写入，本任务读取后通过 UART 发送 ---- */
extern QueueHandle_t uartSensorQueue;    /* 传感器快照队列 */
extern QueueHandle_t remoteControlQueue; /* 远程控制命令队列（本任务写入） */
extern QueueHandle_t controlAckQueue;    /* 控制应答队列 */
extern QueueHandle_t deviceStateQueue;   /* 设备状态队列 */

/* ---- 发送序号：每发一帧自动递增，用于 ESP8266 端做消息去重/排序 ---- */
static uint8_t tx_sequence;
/* ---- 心跳状态：用于判断 ESP8266 是否在线 ---- */
static TickType_t last_heartbeat_tick;   /* 最近一次收到心跳的时间戳 */
static uint8_t heartbeat_received;       /* 是否至少收到过一次心跳 */

/**
 * @brief 检查 ESP8266 是否在线
 *        判断依据：至少收到过一次心跳，且距上次心跳不超过 5 秒
 */
bool Uart_IsEspOnline(void)
{
    if (heartbeat_received == 0U) {
        return false;   /* 从未收到心跳，视为离线 */
    }
    /* 利用 TickType_t 无符号减法自动处理溢出 */
    return (xTaskGetTickCount() - last_heartbeat_tick) < pdMS_TO_TICKS(5000);
}

/* ---- 小端序打包工具：将多字节数值按 Little-Endian 写入 payload 缓冲区 ---- */
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

/**
 * @brief 编码并发送一帧协议数据
 * @param type     消息类型（心跳/传感器/控制/应答等）
 * @param sequence 帧序号，用于对端匹配请求与响应
 * @param payload  载荷指针
 * @param length   载荷长度
 */
static void send_frame(uint8_t type, uint8_t sequence,
                       const uint8_t *payload, uint16_t length)
{
    uint8_t frame[BSP_PROTOCOL_MAX_FRAME];
    /* 编码：加上帧头、CRC 等，结果写入 frame[] */
    size_t frame_length = BSP_Protocol_Encode(type, sequence, payload, length,
                                               frame, sizeof(frame));
    if (frame_length != 0U) {
        /* 超时 20 tick 发送，避免阻塞太久 */
        (void)BSP_UartBridge_Write(frame, frame_length, 20U);
    }
}

/**
 * @brief 打包传感器快照并通过 UART 发送给 ESP8266
 *        payload 布局（24 字节，全部小端序）：
 *        [0-3]  timestamp_ms   4B  采样时间戳
 *        [4-7]  valid_mask     4B  有效传感器位掩码
 *        [8-9]  temperature    2B  温度（0.01℃ 精度）
 *        [10-11] humidity      2B  湿度（0.01% 精度）
 *        [12-15] pressure      4B  气压（Pa）
 *        [16-17] light_raw     2B  光敏 ADC 原始值
 *        [18-19] mq2_raw       2B  MQ-2 烟雾传感器 ADC 值
 *        [20-21] mq7_raw       2B  MQ-7 一氧化碳传感器 ADC 值
 *        [22-23] mq135_raw     2B  MQ-135 空气质量传感器 ADC 值
 */
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

/**
 * @brief 发送设备状态（模式、灯、门锁）给 ESP8266
 *        payload：[0] mode, [1] light_on, [2] door_open
 */
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

/**
 * @brief 处理从 ESP8266 收到的完整协议帧
 *        - 心跳帧：更新在线状态并原样回复
 *        - 控制命令帧：投递到 remoteControlQueue，队列满则立即回 BUSY ACK
 */
static void handle_frame(const bsp_protocol_frame_t *frame)
{
    if (frame->type == BSP_MSG_HEARTBEAT) {
        /* 记录心跳时间，标记 ESP8266 在线 */
        last_heartbeat_tick = xTaskGetTickCount();
        heartbeat_received = 1U;
        /* ESP8266 主动发心跳；STM32 原样回送 payload 和 sequence。 */
        send_frame(BSP_MSG_HEARTBEAT, frame->sequence,
                   frame->payload, frame->length);

    } else if (frame->type == BSP_MSG_CONTROL_COMMAND && frame->length == 2U) {
        /* 控制命令 payload：[0]=device, [1]=value */
        app_control_command_t command;
        command.sequence = frame->sequence;   /* 保留原序号，便于 ACK 匹配 */
        command.device = frame->payload[0];
        command.value = frame->payload[1];

        /* 非阻塞投递，队列满说明 control_task 忙，直接回 BUSY */
        if (xQueueSend(remoteControlQueue, &command, 0) != pdTRUE) {
            app_control_ack_t ack = {
                frame->sequence, APP_ACK_BUSY, command.device, command.value
            };
            send_control_ack(&ack);
        }
    }
}

/**
 * @brief UART 通信主任务
 *        职责：
 *        1. 持续接收 ESP8266 数据，逐字节喂给协议解析器，完整帧交给 handle_frame
 *        2. 轮询 sensor/deviceState/controlAck 队列，将最新状态打包发给 ESP8266
 *        运行周期约 5ms，兼顾实时性与 CPU 占用
 */
void Uart_Task(void *params)
{
    bsp_protocol_parser_t parser;          /* 协议解析状态机 */
    bsp_protocol_frame_t received_frame;   /* 解析出的完整帧 */
    bsp_sensor_snapshot_t sensor;          /* 从队列读取的传感器快照 */
    app_control_ack_t ack;                 /* 从队列读取的控制应答 */
    app_device_state_t state;              /* 从队列读取的设备状态 */
    uint8_t rx_data[32];                   /* UART 接收缓冲区 */

    (void)params;
    BSP_Protocol_ParserInit(&parser);      /* 把used清零 */

    for (;;) {
        /* ---- 第一步：接收并解析 ESP8266 发来的数据 ---- */
        size_t count = BSP_UartBridge_Read(rx_data, sizeof(rx_data));  // rx_data 存原始字节
        for (size_t i = 0; i < count; ++i) {
            /* parser 逐字节消化，received_frame 存结果 */
            if (BSP_Protocol_Feed(&parser, rx_data[i], &received_frame) == BSP_OK) {
                handle_frame(&received_frame);  // received_frame 交给处理函数
            }
        }

        /* ---- 第二步：将各队列中的最新数据发送给 ESP8266 ---- */
        /* 队列深度为 1，只保留最新值；循环取走所有积压数据 */
        while (xQueueReceive(uartSensorQueue, &sensor, 0) == pdTRUE) {
            send_sensor_report(&sensor);   // sensor 存传感器数据，打包发给 ESP
        }
        while (xQueueReceive(deviceStateQueue, &state, 0) == pdTRUE) {
            send_device_state(&state);     // state 存设备状态，打包发给 ESP
        }
        while (xQueueReceive(controlAckQueue, &ack, 0) == pdTRUE) {
            send_control_ack(&ack);        // ack 存控制应答，打包发给 ESP
        }

        vTaskDelay(pdMS_TO_TICKS(5));   /* 5ms 轮询周期 */
    }
}
