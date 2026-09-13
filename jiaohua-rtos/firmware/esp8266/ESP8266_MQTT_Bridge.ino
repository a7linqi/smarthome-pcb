  /*
 * ESP8266_MQTT_Bridge  —  串口 <-> MQTT 桥（免费用真 MQTT）
 *
 * 作用：
 *   对 STM32 侧：仍然假装成一个 AT 模块（所以 STM32 代码不用改）
 *   对网络侧：用真正的 MQTT 协议连免费 broker
 *     - STM32 上报的 cmd=2...topic=data&msg=xxx   -> MQTT PUBLISH 到数据主题
 *     - MQTT 控制主题收到消息                      -> 原样转发给 STM32
 *
 * 免费 broker 两种可选（改下面 BROKER_CHOICE）：
 *   1) BROKER_PUBLIC : broker.emqx.io:1883  —— 公共免费，不用注册（推荐先用这个）
 *   2) BROKER_BEMFA  : bemfa.com:9501       —— 你自己的巴法云 MQTT 账号（免费，
 *                       需先在巴法云"MQTT设备云"里创建 data / control 两个主题）
 *
 * 需要安装库：PubSubClient（Arduino IDE -> 工具 -> 管理库 -> 搜索 PubSubClient -> 安装）
 * 波特率：115200（和 STM32 的 USART2 一致）
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "bridge_config.h"

// ================= 配置 =================
#define BROKER_PUBLIC 2
#define BROKER_BEMFA  1
#define BROKER_CHOICE BROKER_PUBLIC      // 用免费公共 broker（无需注册/无需建主题）

#if BROKER_CHOICE == BROKER_BEMFA
  const char* MQTT_HOST    = "mqtt.bemfa.com";
  const uint16_t MQTT_PORT = 9501;
  const char* MQTT_USER    = BRIDGE_BEMFA_USER;
  const char* MQTT_PASS    = BRIDGE_BEMFA_PASSWORD;
  const char* TOPIC_DATA    = "data";
  const char* TOPIC_CONTROL = "control";
#else
  const char* MQTT_HOST    = "broker.emqx.io";   // 公共免费 broker
  const uint16_t MQTT_PORT = 1883;
  const char* MQTT_USER    = "";
  const char* MQTT_PASS    = "";
  const char* TOPIC_DATA    = "jiaohua/" BRIDGE_TOPIC_NAMESPACE "/data";
  const char* TOPIC_CONTROL = "jiaohua/" BRIDGE_TOPIC_NAMESPACE "/control";
#endif
// =======================================

WiFiClient   net;
PubSubClient mqtt(net);

constexpr unsigned long MQTT_RETRY_MS = 5000UL;
constexpr size_t SERIAL_LINE_MAX = 256U;

uint8_t  wifiState = 0;          // 0未开始 1连接中 2已连接 3失败
unsigned long wifiStart = 0;
bool     bridge = false;         // 是否已进入"透传"(桥接)模式
String   rxLine = "";
String   savedSsid, savedPass;
unsigned long lastMqttAttempt = 0;
bool mqttStateReported = false;

void replyOK()  { Serial.print("OK\r\n"); }
void replyErr() { Serial.print("ERROR\r\n"); }

// MQTT 收到消息 -> 转发给 STM32（换行结尾，方便 STM32 解析指令）
void onMqtt(char* topic, byte* payload, unsigned int len) {
  for (unsigned int i = 0; i < len; i++) Serial.write(payload[i]);
  Serial.print("\r\n");
}

bool mqttConnect() {
  if (WiFi.status() != WL_CONNECTED) return false;
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqtt);
  mqtt.setKeepAlive(60);

  char clientId[32];
  snprintf(clientId, sizeof(clientId), "jiaohua-%06X", ESP.getChipId());

  bool ok;
  if (strlen(MQTT_USER) == 0) ok = mqtt.connect(clientId);
  else                        ok = mqtt.connect(clientId, MQTT_USER, MQTT_PASS);

  if (ok) {
    mqtt.subscribe(TOPIC_CONTROL);   // 只订阅控制主题
    if (bridge && !mqttStateReported) Serial.print("+MQTT:ONLINE\r\n");
    mqttStateReported = true;
  }
  return ok;
}

void maintainMqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    if (mqttStateReported) Serial.print("+MQTT:OFFLINE\r\n");
    mqttStateReported = false;
    return;
  }

  if (mqtt.connected()) {
    mqtt.loop();
    return;
  }

  if (mqttStateReported) Serial.print("+MQTT:OFFLINE\r\n");
  mqttStateReported = false;

  const unsigned long now = millis();
  if (now - lastMqttAttempt >= MQTT_RETRY_MS) {
    lastMqttAttempt = now;
    mqttConnect();
  }
}

// ---------- WiFi 连接（非阻塞，连上才回 OK） ----------
void startWifi(const String& s, const String& p) {
  savedSsid = s; savedPass = p;
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  wifiState = 1; wifiStart = millis();
}

void pollWifi() {
  if (wifiState == 1) {
    if (WiFi.status() == WL_CONNECTED) { wifiState = 2; replyOK(); return; }
    if (millis() - wifiStart > 12000) { WiFi.disconnect(); wifiState = 3; replyErr(); }
  }
}

// ---------- AT 指令处理（给 STM32 用） ----------
void handleAt(String& s) {
  if (s.startsWith("AT+CIPSTART")) {
    if (WiFi.status() == WL_CONNECTED && mqttConnect()) Serial.print("CONNECT\r\nOK\r\n");
    else replyErr();
  } else if (s.startsWith("AT+CIPSEND")) {
    bridge = true;                 // 进入桥接模式
    replyOK();
    Serial.print(mqtt.connected() ? "+MQTT:ONLINE\r\n"
                                  : "+MQTT:OFFLINE\r\n");
  } else if (s.startsWith("AT+CWJAP")) {
    int q1 = s.indexOf('"');
    int q2 = s.indexOf('"', q1 + 1);
    int q3 = s.indexOf('"', q2 + 1);
    int q4 = s.indexOf('"', q3 + 1);
    if (q1 < 0 || q4 < 0) { replyOK(); return; }
    if (wifiState == 1) return;                       // 连接中，忽略重复指令
    if (wifiState == 2) { replyOK(); return; }        // 已连接
    startWifi(s.substring(q1 + 1, q2), s.substring(q3 + 1, q4));
  } else {
    replyOK();                     // AT / CWMODE / CIPMODE 等一律 OK
  }
}

// ---------- 桥接模式：解析 STM32 发来的数据 ----------
void handleBridge(String& s) {
  if (s.startsWith("cmd=1")) {
    // STM32 的订阅指令：我们内部已经订阅好了，直接回"订阅成功"
    Serial.print("cmd=1&res=1\r\n");
    mqtt.subscribe(TOPIC_CONTROL);
    return;
  }
  if (s.startsWith("cmd=2")) {
    // 形如 cmd=2&uid=..&topic=data&msg=#..#sensordata#
    int ti = s.indexOf("topic=");
    int mi = s.indexOf("msg=");
    if (ti < 0 || mi < 0) return;
    int te = s.indexOf('&', ti);
    String topic = (te > 0) ? s.substring(ti + 6, te) : s.substring(ti + 6);
    String msg   = s.substring(mi + 4);
    const char* outTopic = TOPIC_DATA;
    if (topic == "control") outTopic = TOPIC_CONTROL;
    if (mqtt.connected()) mqtt.publish(outTopic, msg.c_str());
    return;
  }
  // 其他内容忽略
}

void setup() {
  Serial.begin(115200);
  rxLine.reserve(SERIAL_LINE_MAX);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(100);
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      if (bridge) handleBridge(rxLine); else handleAt(rxLine);
      rxLine = "";
    } else if (c != '\r') {
      if (rxLine.length() < SERIAL_LINE_MAX) {
        rxLine += c;
      } else {
        rxLine = "";
      }
    }
  }

  if (!bridge) pollWifi();
  else maintainMqtt();   // 掉线后每5秒尝试一次，避免阻塞主循环
}
