#ifndef BSP_PROTOCOL_H
#define BSP_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include "bsp_status.h"

#define BSP_PROTOCOL_VERSION 1U
#define BSP_PROTOCOL_MAX_PAYLOAD 96U
#define BSP_PROTOCOL_MAX_FRAME (BSP_PROTOCOL_MAX_PAYLOAD + 10U)

typedef enum {
    BSP_MSG_HEARTBEAT = 0x01,
    BSP_MSG_SENSOR_REPORT = 0x10,
    BSP_MSG_DEVICE_STATE = 0x11,
    BSP_MSG_CONTROL_COMMAND = 0x20,
    BSP_MSG_CONTROL_ACK = 0x21,
    BSP_MSG_ERROR = 0x7F
} bsp_message_type_t;

typedef struct {
    uint8_t type;
    uint8_t sequence;
    uint16_t length;
    uint8_t payload[BSP_PROTOCOL_MAX_PAYLOAD];
} bsp_protocol_frame_t;

typedef struct {
    uint8_t buffer[BSP_PROTOCOL_MAX_FRAME];
    uint16_t used;
} bsp_protocol_parser_t;

uint16_t BSP_Protocol_Crc16(const uint8_t *data, size_t length);
size_t BSP_Protocol_Encode(uint8_t type, uint8_t sequence, const void *payload,
                           uint16_t length, uint8_t *output, size_t capacity);
void BSP_Protocol_ParserInit(bsp_protocol_parser_t *parser);
bsp_status_t BSP_Protocol_Feed(bsp_protocol_parser_t *parser, uint8_t byte,
                               bsp_protocol_frame_t *frame);

#endif
