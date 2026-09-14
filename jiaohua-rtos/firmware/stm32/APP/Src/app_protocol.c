#include "app_protocol.h"

#include <string.h>

#define APP_PROTOCOL_SOF0        0xA5U
#define APP_PROTOCOL_SOF1        0x5AU
#define APP_PROTOCOL_HEADER_SIZE 8U

uint16_t AppProtocol_Crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFU;

    while (length-- > 0U) {
        uint8_t bit;

        crc ^= (uint16_t)(*data++) << 8;
        for (bit = 0U; bit < 8U; ++bit) {
            crc = (crc & 0x8000U)
                      ? (uint16_t)((crc << 1) ^ 0x1021U)
                      : (uint16_t)(crc << 1);
        }
    }

    return crc;
}

size_t AppProtocol_Encode(uint8_t type, uint8_t sequence,
                          const uint8_t *payload, uint16_t length,
                          uint8_t *output, size_t capacity)
{
    const size_t total = APP_PROTOCOL_HEADER_SIZE + length + 2U;
    uint16_t crc;

    if ((output == NULL) || (length > APP_PROTOCOL_MAX_PAYLOAD) ||
        (capacity < total) || ((length > 0U) && (payload == NULL))) {
        return 0U;
    }

    output[0] = APP_PROTOCOL_SOF0;
    output[1] = APP_PROTOCOL_SOF1;
    output[2] = APP_PROTOCOL_VERSION;
    output[3] = type;
    output[4] = sequence;
    output[5] = 0U;
    AppProtocol_PutU16(output + 6U, length);

    if (length > 0U) {
        memcpy(output + APP_PROTOCOL_HEADER_SIZE, payload, length);
    }

    crc = AppProtocol_Crc16(output + 2U,
                            APP_PROTOCOL_HEADER_SIZE - 2U + length);
    AppProtocol_PutU16(output + APP_PROTOCOL_HEADER_SIZE + length, crc);

    return total;
}

void AppProtocol_ParserInit(AppProtocolParser *parser)
{
    if (parser != NULL) {
        parser->used = 0U;
    }
}

AppParseResult AppProtocol_Feed(AppProtocolParser *parser, uint8_t byte,
                                AppProtocolFrame *frame)
{
    uint16_t length;
    size_t total;
    uint16_t received_crc;
    uint16_t expected_crc;

    if ((parser == NULL) || (frame == NULL)) {
        return APP_PARSE_ERROR;
    }

    if ((parser->used == 0U) && (byte != APP_PROTOCOL_SOF0)) {
        return APP_PARSE_WAITING;
    }

    if ((parser->used == 1U) && (byte != APP_PROTOCOL_SOF1)) {
        parser->used = (byte == APP_PROTOCOL_SOF0) ? 1U : 0U;
        return APP_PARSE_WAITING;
    }

    if (parser->used >= sizeof(parser->buffer)) {
        parser->used = 0U;
        return APP_PARSE_ERROR;
    }

    parser->buffer[parser->used++] = byte;
    if (parser->used < APP_PROTOCOL_HEADER_SIZE) {
        return APP_PARSE_WAITING;
    }

    length = AppProtocol_GetU16(parser->buffer + 6U);
    if ((parser->buffer[2] != APP_PROTOCOL_VERSION) ||
        (length > APP_PROTOCOL_MAX_PAYLOAD)) {
        parser->used = 0U;
        return APP_PARSE_ERROR;
    }

    total = APP_PROTOCOL_HEADER_SIZE + length + 2U;
    if (parser->used < total) {
        return APP_PARSE_WAITING;
    }

    received_crc = AppProtocol_GetU16(
        parser->buffer + APP_PROTOCOL_HEADER_SIZE + length);
    expected_crc = AppProtocol_Crc16(
        parser->buffer + 2U, APP_PROTOCOL_HEADER_SIZE - 2U + length);
    if (received_crc != expected_crc) {
        parser->used = 0U;
        return APP_PARSE_ERROR;
    }

    frame->type = parser->buffer[3];
    frame->sequence = parser->buffer[4];
    frame->length = length;
    if (length > 0U) {
        memcpy(frame->payload,
               parser->buffer + APP_PROTOCOL_HEADER_SIZE,
               length);
    }

    parser->used = 0U;
    return APP_PARSE_FRAME;
}

uint16_t AppProtocol_GetU16(const uint8_t *source)
{
    return (uint16_t)source[0] | ((uint16_t)source[1] << 8);
}

void AppProtocol_PutU16(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8);
}
