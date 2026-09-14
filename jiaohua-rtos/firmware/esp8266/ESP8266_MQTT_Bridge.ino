/* ESP8266 UART <-> MQTT bridge for the watering controller.
 * The ESP8266 runs this custom firmware; no AT firmware is required. */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "bridge_config.h"
#include "protocol.h"

#define BROKER_PUBLIC 2
#define BROKER_BEMFA  1
#define BROKER_CHOICE BROKER_PUBLIC

#if BROKER_CHOICE == BROKER_BEMFA
const char *MQTT_HOST = "mqtt.bemfa.com";
const uint16_t MQTT_PORT = 9501;
const char *MQTT_USER = BRIDGE_BEMFA_USER;
const char *MQTT_PASS = BRIDGE_BEMFA_PASSWORD;
const char *TOPIC_DATA = "data";
const char *TOPIC_CONTROL = "control";
const char *TOPIC_ACK = "control_ack";
#else
const char *MQTT_HOST = "broker.emqx.io";
const uint16_t MQTT_PORT = 1883;
const char *MQTT_USER = "";
const char *MQTT_PASS = "";
const char *TOPIC_DATA = "jiaohua/" BRIDGE_TOPIC_NAMESPACE "/data";
const char *TOPIC_CONTROL = "jiaohua/" BRIDGE_TOPIC_NAMESPACE "/control";
const char *TOPIC_ACK = "jiaohua/" BRIDGE_TOPIC_NAMESPACE "/ack";
#endif

constexpr unsigned long WIFI_RETRY_MS = 10000UL;
constexpr unsigned long MQTT_RETRY_MS = 5000UL;

WiFiClient networkClient;
PubSubClient mqtt(networkClient);
Protocol::Parser uartParser;

unsigned long lastWifiAttempt;
unsigned long lastMqttAttempt;
bool reportedMqttOnline;
uint8_t txSequence;

void sendFrame(uint8_t type, uint8_t sequence,
               const uint8_t *payload, uint16_t length)
{
    uint8_t output[Protocol::MAX_FRAME];
    const size_t count = Protocol::encode(type, sequence, payload, length,
                                          output, sizeof(output));

    if (count > 0U) {
        Serial.write(output, count);
    }
}

void reportMqttStatus(bool online)
{
    const uint8_t payload[1] = {online ? 1U : 0U};
    sendFrame(Protocol::MQTT_STATUS, txSequence++, payload, sizeof(payload));
    reportedMqttOnline = online;
}

bool decodeControl(const byte *payload, unsigned int length,
                   uint8_t &command, uint16_t &value)
{
    char text[64];
    const size_t count = (length < sizeof(text) - 1U)
                             ? length
                             : sizeof(text) - 1U;

    memcpy(text, payload, count);
    text[count] = '\0';
    value = 0U;

    if (strcmp(text, "SET_MODE:ZD") == 0) {
        command = 1U;
    } else if (strcmp(text, "SET_MODE:SD") == 0) {
        command = 2U;
    } else if (strcmp(text, "Water_pump_ON") == 0) {
        command = 3U;
    } else if (strcmp(text, "Water_pump_OFF") == 0) {
        command = 4U;
    } else if (strncmp(text, "SET_T_H:", 8U) == 0) {
        command = 5U;
        value = static_cast<uint16_t>(atoi(text + 8U));
    } else if (strncmp(text, "SET_H_L:", 8U) == 0) {
        command = 6U;
        value = static_cast<uint16_t>(atoi(text + 8U));
    } else {
        return false;
    }

    return true;
}

void onMqttMessage(char *topic, byte *payload, unsigned int length)
{
    uint8_t command;
    uint16_t value;
    uint8_t framePayload[3];

    if ((strcmp(topic, TOPIC_CONTROL) != 0) ||
        !decodeControl(payload, length, command, value)) {
        return;
    }

    framePayload[0] = command;
    framePayload[1] = static_cast<uint8_t>(value);
    framePayload[2] = static_cast<uint8_t>(value >> 8);
    sendFrame(Protocol::CONTROL_COMMAND, txSequence++, framePayload,
              sizeof(framePayload));
}

bool connectMqtt()
{
    char clientId[32];
    bool connected;

    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    snprintf(clientId, sizeof(clientId), "jiaohua-%06X", ESP.getChipId());
    if (strlen(MQTT_USER) == 0U) {
        connected = mqtt.connect(clientId);
    } else {
        connected = mqtt.connect(clientId, MQTT_USER, MQTT_PASS);
    }

    if (connected) {
        mqtt.subscribe(TOPIC_CONTROL);
        reportMqttStatus(true);
    }

    return connected;
}

void maintainConnections()
{
    const unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (reportedMqttOnline) {
            reportMqttStatus(false);
        }
        if (now - lastWifiAttempt >= WIFI_RETRY_MS) {
            lastWifiAttempt = now;
            WiFi.begin(BRIDGE_WIFI_SSID, BRIDGE_WIFI_PASSWORD);
        }
        return;
    }

    if (!mqtt.connected()) {
        if (reportedMqttOnline) {
            reportMqttStatus(false);
        }
        if (now - lastMqttAttempt >= MQTT_RETRY_MS) {
            lastMqttAttempt = now;
            connectMqtt();
        }
        return;
    }

    mqtt.loop();
}

void publishTelemetry(const Protocol::Frame &frame)
{
    char json[160];

    if (frame.length != 11U) {
        return;
    }

    snprintf(json, sizeof(json),
             "{\"temperature\":%u,\"humidity\":%u,\"soil\":%u,"
             "\"soil_adc\":%u,\"mode\":%u,\"pump\":%u,\"alarm\":%u}",
             Protocol::getU16LE(frame.payload + 0U),
             Protocol::getU16LE(frame.payload + 2U),
             Protocol::getU16LE(frame.payload + 4U),
             Protocol::getU16LE(frame.payload + 6U),
             frame.payload[8], frame.payload[9], frame.payload[10]);
    mqtt.publish(TOPIC_DATA, json, true);
}

void publishControlAck(const Protocol::Frame &frame)
{
    char json[96];

    if (frame.length != 4U) {
        return;
    }

    snprintf(json, sizeof(json),
             "{\"sequence\":%u,\"command\":%u,\"status\":%u,\"value\":%u}",
             frame.sequence, frame.payload[0], frame.payload[1],
             Protocol::getU16LE(frame.payload + 2U));
    mqtt.publish(TOPIC_ACK, json, false);
}

void handleUartFrame(const Protocol::Frame &frame)
{
    if ((frame.type == Protocol::TELEMETRY) && mqtt.connected()) {
        publishTelemetry(frame);
    } else if ((frame.type == Protocol::CONTROL_ACK) && mqtt.connected()) {
        publishControlAck(frame);
    } else if (frame.type == Protocol::HEARTBEAT) {
        sendFrame(Protocol::HEARTBEAT, frame.sequence, nullptr, 0U);
    }
}

void setup()
{
    Serial.begin(115200);
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(BRIDGE_WIFI_SSID, BRIDGE_WIFI_PASSWORD);

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(onMqttMessage);
    mqtt.setKeepAlive(60U);
    mqtt.setBufferSize(256U);

    lastWifiAttempt = millis();
    lastMqttAttempt = 0U;
    reportedMqttOnline = false;
}

void loop()
{
    Protocol::Frame frame;

    maintainConnections();

    while (Serial.available() > 0) {
        if (uartParser.feed(static_cast<uint8_t>(Serial.read()), frame)) {
            handleUartFrame(frame);
        }
    }
}
