#include <ESP8266WiFi.h>
#include "wifi_config.h"

/* ========================================
 * WiFi 连接测试程序
 * 功能：验证 ESP8266 能否连接 WiFi 路由器
 * ======================================== */

/* ---------- 全局变量 ---------- */
static const unsigned long RETRY_MS = 20000UL;  // 重试间隔：20秒
static unsigned long last_attempt_ms;            // 上次尝试连接的时间
static unsigned long last_report_ms;             // 上次报告状态的时间
static bool connected_before = false;            // 上次是否已连接
static bool configured = false;                  // 是否已配置WiFi

/* ---------- 启动连接函数 ---------- */
/* 作用：开始连接 WiFi 路由器 */
static void start_connection()
{
    last_attempt_ms = millis();  // 记录开始连接的时间
    Serial.println(F("[WiFi] Connecting..."));  // 打印连接提示
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // 连接 WiFi（名称和密码）
}

/* ---------- setup() ---------- */
/* 作用：程序启动时执行一次，初始化串口和 WiFi */
void setup()
{
    Serial.begin(115200);           // 初始化串口，波特率115200
    Serial.setDebugOutput(false);   // 关闭调试输出
    delay(200);                     // 等待串口稳定
    Serial.println(F("\nSmartHome WiFi test"));  // 打印程序名

    // WiFi配置：不保存到Flash，STA模式，不自动重连
    WiFi.persistent(false);        // 不保存WiFi配置到Flash
    WiFi.mode(WIFI_STA);           // STA模式（连接路由器）
    WiFi.setAutoReconnect(false);  // 不自动重连（由代码控制）
    WiFi.disconnect();             // 先断开

    // 检查是否配置了WiFi
    configured = WIFI_SSID[0] != '\0';
    if (!configured) {
        Serial.println(F("[Config] Fill WIFI_SSID and WIFI_PASSWORD in wifi_config.h, then upload again."));
        return;  // 没配置，直接返回
    }
    start_connection();  // 开始连接
}

/* ---------- loop() ---------- */
/* 作用：主循环，反复执行，处理WiFi连接状态 */
void loop()
{
    // 如果没配置WiFi，延迟后返回
    if (!configured) {
        delay(10);
        return;
    }

    const unsigned long now = millis();
    const bool connected = WiFi.status() == WL_CONNECTED;  // 当前连接状态

    // 检测连接状态变化
    if (connected && !connected_before) {
        // 刚连上：打印IP地址
        Serial.print(F("[WiFi] Connected. IP="));
        Serial.println(WiFi.localIP());
    } else if (!connected && connected_before) {
        // 刚断开：提示并重连
        Serial.println(F("[WiFi] Disconnected; retrying."));
        WiFi.disconnect();
        start_connection();
    }
    connected_before = connected;  // 更新状态

    // 超时重试：如果没连接且超过20秒
    if (!connected && millis() - last_attempt_ms >= RETRY_MS) {
        Serial.println(F("[WiFi] Timeout. Check 2.4GHz network, password and power."));
        WiFi.disconnect();
        start_connection();
    }

    // 每5秒报告一次WiFi状态
    if (now - last_report_ms >= 5000UL) {
        last_report_ms = now;
        Serial.print(F("[WiFi] status="));
        Serial.print((int)WiFi.status());
        if (connected) {
            Serial.print(F(" RSSI="));  // 信号强度
            Serial.print(WiFi.RSSI());
        }
        Serial.println();
    }

    delay(1);  // 微小延迟，降低CPU功耗
}
