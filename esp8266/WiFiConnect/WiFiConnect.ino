#include <ESP8266WiFi.h>
#include "wifi_config.h"

// 第一阶段：仅验证联网。Serial 是 USB-TTL 调试输出，暂不接 STM32 协议。
static const unsigned long RETRY_MS = 20000UL;
static unsigned long last_attempt_ms;
static unsigned long last_report_ms;
static bool connected_before = false;
static bool configured = false;

static void start_connection()
{
    last_attempt_ms = millis();
    Serial.println(F("[WiFi] Connecting..."));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    delay(200);
    Serial.println(F("\nSmartHome WiFi test"));

    // 重试不反复写 Flash；显式使用 station 模式。
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // 由 loop 统一按间隔重试。
    WiFi.disconnect();
    configured = WIFI_SSID[0] != '\0';
    if (!configured) {
        Serial.println(F("[Config] Fill WIFI_SSID and WIFI_PASSWORD in wifi_config.h, then upload again."));
        return;
    }
    start_connection();
}

void loop()
{
    if (!configured) {
        delay(10);
        return;
    }
    const unsigned long now = millis();
    const bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !connected_before) {
        Serial.print(F("[WiFi] Connected. IP="));
        Serial.println(WiFi.localIP());
    } else if (!connected && connected_before) {
        Serial.println(F("[WiFi] Disconnected; retrying."));
        WiFi.disconnect();
        start_connection();
    }
    connected_before = connected;

    // 无无限等待循环；网络失败不会阻塞以后加入的串口处理。
    if (!connected && millis() - last_attempt_ms >= RETRY_MS) {
        Serial.println(F("[WiFi] Timeout. Check 2.4GHz network, password and power."));
        WiFi.disconnect();
        start_connection();
    }
    if (now - last_report_ms >= 5000UL) {
        last_report_ms = now;
        Serial.print(F("[WiFi] status="));
        Serial.print((int)WiFi.status());
        if (connected) {
            Serial.print(F(" RSSI="));
            Serial.print(WiFi.RSSI());
        }
        Serial.println();
    }
    delay(1);
}
