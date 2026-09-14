#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define APP_PROTOCOL_VERSION      1U
#define APP_PROTOCOL_MAX_PAYLOAD 32U
#define APP_PROTOCOL_MAX_FRAME   (APP_PROTOCOL_MAX_PAYLOAD + 10U)

typedef enum {
    APP_MSG_HEARTBEAT = 0x01,
    APP_MSG_TELEMETRY = 0x10,
    APP_MSG_CONTROL_COMMAND = 0x20,
    APP_MSG_CONTROL_ACK = 0x21,
    APP_MSG_MQTT_STATUS = 0x30
} AppMessageType;

typedef enum {
    APP_PARSE_WAITING = 0,
    APP_PARSE_FRAME,
    APP_PARSE_ERROR
} AppParseResult;

typedef struct {
    uint8_t type;
    uint8_t sequence;
    uint16_t length;
    uint8_t payload[APP_PROTOCOL_MAX_PAYLOAD];
} AppProtocolFrame;

typedef struct {
    uint8_t buffer[APP_PROTOCOL_MAX_FRAME];
    uint16_t used;
} AppProtocolParser;

uint16_t AppProtocol_Crc16(const uint8_t *data, size_t length);
size_t AppProtocol_Encode(uint8_t type, uint8_t sequence,
                          const uint8_t *payload, uint16_t length,
                          uint8_t *output, size_t capacity);
void AppProtocol_ParserInit(AppProtocolParser *parser);
AppParseResult AppProtocol_Feed(AppProtocolParser *parser, uint8_t byte,
                                AppProtocolFrame *frame);
uint16_t AppProtocol_GetU16(const uint8_t *source);
void AppProtocol_PutU16(uint8_t *destination, uint16_t value);

#endif
