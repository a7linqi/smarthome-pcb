#include <ESP8266WiFi.h>

/* ESP-01S 板载蓝灯通常连接 GPIO2，并且低电平点亮。 */
static const uint8_t LED_PIN = 2;

static unsigned long last_blink_ms = 0;
static bool led_on = false;

static void scan_wifi(void)
{
    Serial.println("Scanning WiFi...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    const int network_count = WiFi.scanNetworks();

    if (network_count <= 0)
    {
        Serial.println("No WiFi found");
    }
    else
    {
        Serial.print("Found WiFi: ");
        Serial.println(network_count);

        for (int index = 0; index < network_count; ++index)
        {
            Serial.print(index + 1);
            Serial.print(". ");
            Serial.print(WiFi.SSID(index));
            Serial.print("  RSSI=");
            Serial.println(WiFi.RSSI(index));
        }
    }

    WiFi.scanDelete();
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("ESP-01S START");

    scan_wifi();

    Serial.println("Test ready. Send text to test UART.");
}

void loop()
{
    const unsigned long now_ms = millis();

    /* 非阻塞闪灯，不影响串口和 Wi-Fi 处理。 */
    if (now_ms - last_blink_ms >= 500UL)
    {
        last_blink_ms = now_ms;
        led_on = !led_on;
        digitalWrite(LED_PIN, led_on ? LOW : HIGH);
    }

    /* 串口回显：收到什么就原样返回什么。 */
    while (Serial.available() > 0)
    {
        Serial.write(Serial.read());
    }

    yield();
}
