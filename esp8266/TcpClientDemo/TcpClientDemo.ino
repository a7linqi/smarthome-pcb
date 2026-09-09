#include <ESP8266WiFi.h>

#include "tcp_config.h"

/* ========================================
 * TCP 客户端示例
 * 功能：ESP8266 连接 WiFi 后，作为 TCP 客户端连接电脑服务器
 * ======================================== */

/*
 * 学习目标：
 * 1. ESP8266 先连接 Wi-Fi 并通过 DHCP 获得 IP；
 * 2. ESP8266 作为 TCP 客户端，连接电脑的 IP 和端口；
 * 3. TCP 连接建立后，定期发送文本并读取服务器回复。
 *
 * 本示例的 Serial 只用于 USB-TTL 调试，测试时不要连接 STM32 协议串口。
 */

/* ---------- 全局变量 ---------- */
static WiFiClient tcp_client;  // TCP 客户端对象

static unsigned long last_wifi_attempt_ms;   // 上次尝试连接WiFi的时间
static unsigned long last_tcp_attempt_ms;    // 上次尝试连接TCP的时间
static unsigned long last_send_ms;           // 上次发送数据的时间
static uint32_t message_number;              // 消息编号（每发一条+1）

/* ========================================
 * WiFi 相关函数
 * ======================================== */

/* ---------- 启动WiFi连接 ---------- */
static void startWiFi(void)
{
    last_wifi_attempt_ms = millis();  // 记录开始连接的时间
    Serial.println(F("[WiFi] connecting"));
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // 连接 WiFi
}

/* ---------- 处理WiFi状态 ---------- */
static void processWiFi(unsigned long now)
{
    // 如果已连接，直接返回
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    // WiFi 断开，关闭 TCP 连接
    if (tcp_client.connected()) {
        tcp_client.stop();
    }

    // 超过20秒还没连上，重试
    if (now - last_wifi_attempt_ms >= 20000UL) {
        WiFi.disconnect();
        startWiFi();
    }
}

/* ========================================
 * TCP 相关函数
 * ======================================== */

/* ---------- 处理TCP连接 ---------- */
static void processTcpConnect(unsigned long now)
{
    // 如果 WiFi 没连上或 TCP 已连接，直接返回
    if (WiFi.status() != WL_CONNECTED || tcp_client.connected()) {
        return;
    }

    // 每5秒尝试一次 TCP 连接
    if (now - last_tcp_attempt_ms < 5000UL) {
        return;
    }

    last_tcp_attempt_ms = now;  // 记录尝试时间

    // 打印连接信息
    Serial.print(F("[TCP] connecting to "));
    Serial.print(TCP_HOST);  // 服务器IP
    Serial.print(':');
    Serial.println(TCP_PORT);  // 服务器端口                    //打印例如[TCP] connecting to 192.168.1.100:8080

    // 尝试连接
    if (tcp_client.connect(TCP_HOST, TCP_PORT)) {
        Serial.println(F("[TCP] connected"));  // 连接成功
    } else {
        Serial.println(F("[TCP] connect failed"));  // 连接失败
        tcp_client.stop();
    }
}

/* ---------- 处理TCP发送 ---------- */
static void processTcpSend(unsigned long now)
{
    // 如果 TCP 没连接或没到3秒，直接返回
    if (!tcp_client.connected() || now - last_send_ms < 3000UL) {
        return;
    }

    last_send_ms = now;  // 记录发送时间

    // 发送消息
    tcp_client.print(F("hello from ESP8266, number="));     //打印给服务器看的（TCP）
    tcp_client.println(message_number++);  // 消息编号+1
    Serial.println(F("[TCP] message sent"));
}

/* ---------- 处理TCP接收 ---------- */
static void processTcpReceive(void)
{
    // 循环读取所有收到的数据
    while (tcp_client.available() > 0) {
        const int value = tcp_client.read();  // 读取一个字节
        if (value >= 0) {
            Serial.write((uint8_t)value);  // 打印到串口
        }
    }
}

/* ========================================
 * 主程序入口
 * ======================================== */

/* ---------- setup() ---------- */
void setup(void)
{
    Serial.begin(115200);           // 初始化串口
    Serial.setDebugOutput(false);   // 关闭调试输出
    delay(200);                     // 等待串口稳定

    // WiFi配置
    WiFi.persistent(false);        // 不保存到Flash
    WiFi.mode(WIFI_STA);           // STA模式
    WiFi.setAutoReconnect(false);  // 不自动重连
    WiFi.disconnect();             // 先断开

    Serial.println(F("\nESP8266 TCP client demo"));
    startWiFi();  // 开始连接 WiFi
}

/* ---------- loop() ---------- */
void loop(void)
{
    const unsigned long now = millis();

    processWiFi(now);  // 处理 WiFi 连接

    // 打印 IP 地址（只打印一次）
    static bool ip_reported;
    if (WiFi.status() == WL_CONNECTED && !ip_reported) {
        ip_reported = true;
        Serial.print(F("[WiFi] connected, local IP="));
        Serial.println(WiFi.localIP());                   //打印后换行
    } else if (WiFi.status() != WL_CONNECTED) {
        ip_reported = false;
    }

    processTcpConnect(now);   // 处理 TCP 连接
    processTcpReceive();      // 处理 TCP 接收
    processTcpSend(now);      // 处理 TCP 发送
    delay(1);                 // 微小延迟
}
