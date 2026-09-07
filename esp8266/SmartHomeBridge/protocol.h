#pragma once

#include <Arduino.h>

namespace Protocol {

constexpr uint8_t VERSION = 1;
constexpr uint16_t MAX_PAYLOAD = 96;
constexpr size_t MAX_FRAME = MAX_PAYLOAD + 10;

enum MessageType : uint8_t {
    HEARTBEAT = 0x01,
    SENSOR_REPORT = 0x10,
    DEVICE_STATE = 0x11,
    CONTROL_COMMAND = 0x20,
    CONTROL_ACK = 0x21,
    ERROR_MESSAGE = 0x7F
};

enum DeviceId : uint8_t {
    DEVICE_LIGHT = 1,
    DEVICE_DOOR = 2,
    DEVICE_MODE = 3
};

struct Frame {
    uint8_t type;
    uint8_t sequence;
    uint16_t length;
    uint8_t payload[MAX_PAYLOAD];
};

class Parser {
public:
    Parser();
    void reset();
    bool feed(uint8_t byte, Frame &frame);

private:
    uint8_t buffer_[MAX_FRAME];
    uint16_t used_;
};

uint16_t crc16(const uint8_t *data, size_t length);
size_t encode(uint8_t type, uint8_t sequence,
              const uint8_t *payload, uint16_t length,
              uint8_t *output, size_t capacity);

uint16_t getU16LE(const uint8_t *src);
uint32_t getU32LE(const uint8_t *src);
void putU32LE(uint8_t *dst, uint32_t value);

} // namespace Protocol
