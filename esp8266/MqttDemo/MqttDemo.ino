#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "mqtt_config.h"

static const char TEST_TOPIC[] = "smarthome/device01/test";

static WiFiClient network_client;
static PubSubClient mqtt_client(network_client);

static unsigned long last_wifi_attempt_ms;
static unsigned long last_mqtt_attempt_ms;
static unsigned long last_publish_ms;
static uint32_t message_number;

static void onMqttMessage(char *topic, uint8_t *payload, unsigned int length)
{
    Serial.print(F("[MQTT] received topic="));
    Serial.print(topic);
    Serial.print(F(" payload="));

    for (unsigned int i = 0; i < length; ++i) {
        Serial.write(payload[i]);
    }
    Serial.println();
}

static void startWiFi(void)
{
    last_wifi_attempt_ms = millis();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

static void processWiFi(unsigned long now)
{
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    if (now - last_wifi_attempt_ms >= 20000UL) {
        WiFi.disconnect();
        startWiFi();
    }
}

static void processMqtt(unsigned long now)
{
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    if (!mqtt_client.connected()) {
        if (now - last_mqtt_attempt_ms < 5000UL) {
            return;
        }

        last_mqtt_attempt_ms = now;
        Serial.println(F("[MQTT] connecting"));
        if (!mqtt_client.connect(MQTT_CLIENT_ID)) {
            Serial.print(F("[MQTT] failed, state="));
            Serial.println(mqtt_client.state());
            return;
        }

        mqtt_client.subscribe(TEST_TOPIC, 1);
        Serial.println(F("[MQTT] connected and subscribed"));
    }

    mqtt_client.loop();
}

static void publishTestMessage(unsigned long now)
{
    if (!mqtt_client.connected() || now - last_publish_ms < 3000UL) {
        return;
    }

    last_publish_ms = now;
    char payload[48];
    snprintf(payload, sizeof(payload), "hello mqtt, number=%lu",
             static_cast<unsigned long>(message_number++));
    mqtt_client.publish(TEST_TOPIC, payload, false);
}

void setup(void)
{
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    delay(200);

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect();

    mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
    mqtt_client.setCallback(onMqttMessage);
    mqtt_client.setKeepAlive(15);
    mqtt_client.setSocketTimeout(2);

    startWiFi();
}

void loop(void)
{
    const unsigned long now = millis();
    processWiFi(now);
    processMqtt(now);
    publishTestMessage(now);
    delay(1);
}
