#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bridge_config.h"
#include "protocol.h"

/*
 * ESP8266 UART0：GPIO1=TX，GPIO3=RX，115200 8N1。
 * 正式运行时 Serial 全部用于二进制协议，不能混入 println 调试文字。
 */

/* ========================================
 * 全局变量区域
 * 这些变量在多个函数之间共享状态
 * ======================================== */

/* ---------- 协议相关变量 ---------- */
static Protocol::Parser parser;           // 帧解析器，逐字节组装完整帧
static Protocol::Frame rx_frame;          // 接收缓冲区，存储解析好的帧
static uint8_t tx_buffer[Protocol::MAX_FRAME]; // 发送缓冲区
static uint8_t next_sequence = 0;         // 帧序号，每发一帧+1，用于匹配请求和响应

/* ---------- MQTT相关变量 ---------- */
static WiFiClient mqtt_network;
static PubSubClient mqtt_client(mqtt_network);
static unsigned long mqtt_attempt_ms = 0;

static const char MQTT_TOPIC_SENSOR[] = "smarthome/device01/sensor";
static const char MQTT_TOPIC_STATE[] = "smarthome/device01/state";
static const char MQTT_TOPIC_COMMAND[] = "smarthome/device01/command";
static const char MQTT_TOPIC_ACK[] = "smarthome/device01/ack";
static const char MQTT_TOPIC_STATUS[] = "smarthome/device01/status";

/* ---------- WiFi 和心跳相关变量 ---------- */
static unsigned long wifi_attempt_ms = 0;       // 上次尝试连接WiFi的时间
static unsigned long heartbeat_ms = 0;          // 上次发送心跳的时间
static unsigned long last_heartbeat_ack_ms = 0; // 上次收到心跳回复的时间
static uint8_t pending_heartbeat_sequence = 0;  // 等待回复的心跳帧序号
static bool waiting_heartbeat_ack = false;       // 是否正在等待心跳回复

/* ========================================
 * 数据结构定义
 * ======================================== */

/* ---------- 传感器数据结构 ---------- */
struct SensorData {
    uint32_t timestamp_ms;          // 时间戳
    uint32_t valid_mask;            // 有效数据位掩码
    int16_t temperature_centi_c;    // 温度（0.01℃为单位）
    uint16_t humidity_centi_pct;    // 湿度（0.01%为单位）
    uint32_t pressure_pa;           // 气压（Pa）
    uint16_t light_raw;             // 光照原始值
    uint16_t mq2_raw;               // MQ2烟雾传感器原始值
    uint16_t mq7_raw;               // MQ7一氧化碳传感器原始值
    uint16_t mq135_raw;             // MQ135空气质量传感器原始值
    bool received;                  // 是否收到过数据
};

/* ---------- 设备状态结构 ---------- */
struct DeviceState {
    uint8_t mode;       // 模式
    uint8_t light_on;   // 灯状态：0=关，1=开
    uint8_t door_open;  // 门状态：0=关，1=开
    bool received;      // 是否收到过数据
};

static SensorData latest_sensor = {};   // 最新的传感器数据
static DeviceState latest_state = {};   // 最新的设备状态

struct ControlAck {
    uint8_t sequence;
    uint8_t status;
    uint8_t device;
    uint8_t value;
};

static ControlAck latest_ack = {};
static bool sensor_dirty = false;  // true表示有一份新数据等待发布到MQTT
static bool state_dirty = false;
static bool ack_dirty = false;

/* ========================================
 * 发送相关函数
 * ======================================== */

/* ---------- 发送帧函数 ---------- */
/* 作用：将数据编码成帧格式，通过串口发送给 STM32 */
static void sendFrame(uint8_t type, uint8_t sequence,
                      const uint8_t *payload, uint16_t length)
{
    // 调用 encode() 函数编码，结果存到 tx_buffer
    const size_t count = Protocol::encode(type, sequence, payload, length,
                                           tx_buffer, sizeof(tx_buffer));
    // 编码成功（count > 0）则发送
    if (count > 0) {
        Serial.write(tx_buffer, count);  // 通过串口发送编码后的帧
    }
}

/* ---------- 发送控制命令 ---------- */
/* 作用：发送控制命令给 STM32（如开灯、关门等） */
/* MQTT 收到控制消息后调用此函数。 */
static void sendControlCommand(uint8_t device, uint8_t value)
{
    // 准备 payload：[设备编号][值]
    const uint8_t payload[2] = {device, value};
    // 发送控制命令，序号+1（next_sequence++ 是先使用后自增）
    sendFrame(Protocol::CONTROL_COMMAND, next_sequence++, payload, sizeof(payload));
}

/* ========================================
 * 接收相关函数
 * ======================================== */

/* ---------- 解码传感器报告 ---------- */
/* 作用：从帧的 payload 中解析出传感器数据 */
static void decodeSensorReport(const Protocol::Frame &frame)
{
    // 检查数据长度是否正确（应该是24字节）
    if (frame.length != 24) {
        return;  // 数据长度不对，丢弃
    }
    // 按偏移量解析各个字段（小端字节序）
    latest_sensor.timestamp_ms = Protocol::getU32LE(frame.payload + 0);   // 偏移0：时间戳
    latest_sensor.valid_mask = Protocol::getU32LE(frame.payload + 4);     // 偏移4：有效掩码
    latest_sensor.temperature_centi_c =
        static_cast<int16_t>(Protocol::getU16LE(frame.payload + 8));      // 偏移8：温度
    latest_sensor.humidity_centi_pct = Protocol::getU16LE(frame.payload + 10);  // 偏移10：湿度
    latest_sensor.pressure_pa = Protocol::getU32LE(frame.payload + 12);   // 偏移12：气压
    latest_sensor.light_raw = Protocol::getU16LE(frame.payload + 16);     // 偏移16：光照
    latest_sensor.mq2_raw = Protocol::getU16LE(frame.payload + 18);       // 偏移18：MQ2
    latest_sensor.mq7_raw = Protocol::getU16LE(frame.payload + 20);       // 偏移20：MQ7
    latest_sensor.mq135_raw = Protocol::getU16LE(frame.payload + 22);     // 偏移22：MQ135
    latest_sensor.received = true;  // 标记已收到数据
    sensor_dirty = true;
}

/* ---------- 处理收到的帧 ---------- */
/* 作用：根据帧类型分发处理 */
static void handleFrame(const Protocol::Frame &frame)
{
    // 根据帧类型进行不同处理
    if (frame.type == Protocol::HEARTBEAT) {
        // 收到心跳回复（STM32 回复 ESP8266 发送的心跳）
        // 检查序号是否匹配，确认是对自己心跳的回复
        if (waiting_heartbeat_ack && frame.sequence == pending_heartbeat_sequence) {
            waiting_heartbeat_ack = false;           // 清除等待标记
            last_heartbeat_ack_ms = millis();        // 记录收到回复的时间
        }
    }
    else if (frame.type == Protocol::SENSOR_REPORT) {
        // 收到传感器报告（STM32 发来的温湿度、气压、气体等数据）
        decodeSensorReport(frame);  // 解码并存到 latest_sensor
    }
    else if (frame.type == Protocol::DEVICE_STATE && frame.length == 3) {
        // 收到设备状态（STM32 发来当前模式、灯、门的状态）
        latest_state.mode = frame.payload[0];       // 模式
        latest_state.light_on = frame.payload[1];   // 灯状态：0=关，1=开
        latest_state.door_open = frame.payload[2];  // 门状态：0=关，1=开
        latest_state.received = true;                // 标记已收到
        state_dirty = true;
    }
    else if (frame.type == Protocol::CONTROL_ACK && frame.length == 3) {
        // 收到控制命令的应答（STM32 执行完命令后回复）
        // payload：[执行状态][设备编号][实际值]
        latest_ack.sequence = frame.sequence;      // 与原控制命令使用同一序号
        latest_ack.status = frame.payload[0];    // 执行状态
        latest_ack.device = frame.payload[1];    // 设备编号
        latest_ack.value = frame.payload[2];     // 实际值
        ack_dirty = true;                        // 等待上报业务执行结果
    }
}

/* ========================================
 * 主循环相关函数
 * ======================================== */

/* ---------- 处理串口数据 ---------- */
/* 作用：从串口读取数据，逐字节喂给解析器，收齐一帧后处理 */
static void processSerial()
{
    /* 每轮限制处理量（最多128字节），让 Wi-Fi 后台和其他逻辑也能获得运行时间。 */
    for (uint16_t count = 0; count < 128 && Serial.available() > 0; ++count) {
        const int value = Serial.read();  // 读取一个字节
        // 逐字节喂给解析器，收齐一帧后处理
        if (value >= 0 && parser.feed(static_cast<uint8_t>(value), rx_frame)) {
            handleFrame(rx_frame);  // 帧完整，处理这帧数据
        }
    }
}

/* ---------- 处理心跳 ---------- */
/* 作用：每2秒发送一次心跳包，保持通信活跃 */
static void processHeartbeat(unsigned long now)
{
    // 每2秒发送一次心跳
    if (now - heartbeat_ms < 2000UL) {
        return;  // 没到2秒，直接返回
    }
    heartbeat_ms = now;  // 更新发送时间

    // 心跳负载：当前时间 + WiFi连接状态
    uint8_t payload[5];
    Protocol::putU32LE(payload, now);  // 前4字节：当前时间（小端序）
    payload[4] = WiFi.status() == WL_CONNECTED ? 1 : 0;  // 第5字节：WiFi状态

    pending_heartbeat_sequence = next_sequence++;  // 记录待确认的心跳序号
    waiting_heartbeat_ack = true;  // 标记正在等待心跳回复

    // 发送心跳帧
    sendFrame(Protocol::HEARTBEAT, pending_heartbeat_sequence,
              payload, sizeof(payload));
}

/* ---------- 启动WiFi连接 ---------- */
/* 作用：开始连接 WiFi 路由器 */
static void startWiFi()
{
    wifi_attempt_ms = millis();  // 记录开始连接的时间
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  // 连接 WiFi
}

/* ---------- 处理WiFi重连 ---------- */
/* 作用：WiFi 断开后自动重连 */
static void processWiFi(unsigned long now)
{
    // 如果没配置WiFi或已连接，直接返回
    if (WIFI_SSID[0] == '\0' || WiFi.status() == WL_CONNECTED) {
        return;
    }
    // 断开超过20秒自动重连
    if (now - wifi_attempt_ms >= 20000UL) {
        WiFi.disconnect();      // 先断开（清理残留状态）
        startWiFi();            // 重新连接
    }
}

/* ---------- 从简单JSON中读取0~255的整数 ---------- */
/*
 * 支持的命令格式：{"device":1,"value":1}
 * 这里只解析两个整数，避免为了很小的消息额外引入大型JSON库。
 */
static bool findJsonByte(const char *json, const char *key, uint8_t &result)
{
    const char *position = strstr(json, key);
    if (position == nullptr) {
        return false;
    }

    position = strchr(position + strlen(key), ':');
    if (position == nullptr) {
        return false;
    }

    char *end = nullptr;
    const long value = strtol(position + 1, &end, 10);
    if (end == position + 1 || value < 0 || value > 255) {
        return false;
    }

    result = static_cast<uint8_t>(value);
    return true;
}

/* ---------- MQTT消息回调 ---------- */
/* Broker把订阅到的command消息交给这里，再转换成UART控制帧。 */
static void onMqttMessage(char *topic, uint8_t *payload, unsigned int length)
{
    if (strcmp(topic, MQTT_TOPIC_COMMAND) != 0 || length == 0 || length >= 96) {
        return;
    }

    char json[96];
    memcpy(json, payload, length);
    json[length] = '\0';

    uint8_t device = 0;
    uint8_t value = 0;
    if (!findJsonByte(json, "\"device\"", device) ||
        !findJsonByte(json, "\"value\"", value)) {
        return;
    }

    if (device < Protocol::DEVICE_LIGHT || device > Protocol::DEVICE_MODE || value > 1) {
        return;
    }

    sendControlCommand(device, value);
}

/* ---------- 建立和维持MQTT连接 ---------- */
static void processMqtt(unsigned long now)
{
    if (WiFi.status() != WL_CONNECTED) {
        if (mqtt_network.connected()) {
            mqtt_network.stop();
        }
        return;
    }

    if (!mqtt_client.connected()) {
        if (mqtt_attempt_ms != 0 && now - mqtt_attempt_ms < 5000UL) {
            return;
        }
        mqtt_attempt_ms = now;

        bool connected;
        if (MQTT_USERNAME[0] != '\0') {
            connected = mqtt_client.connect(
                MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD,
                MQTT_TOPIC_STATUS, 1, true, "offline");
        } else {
            connected = mqtt_client.connect(
                MQTT_CLIENT_ID, MQTT_TOPIC_STATUS, 1, true, "offline");
        }

        if (!connected) {
            return;
        }

        if (!mqtt_client.subscribe(MQTT_TOPIC_COMMAND, 1)) {
            mqtt_client.disconnect();
            return;
        }
        (void)mqtt_client.publish(MQTT_TOPIC_STATUS, "online", true);

        // 重连后再次上报最近状态；传感器数据有新值时也会补发。
        state_dirty = latest_state.received;
        sensor_dirty = latest_sensor.received;
    }

    mqtt_client.loop();  // 收命令、维持Keep Alive，必须频繁调用
}

/* ---------- 发布STM32送来的最新数据 ---------- */
static void publishPendingMqtt()
{
    if (!mqtt_client.connected()) {
        return;
    }

    char json[256];

    if (sensor_dirty) {
        snprintf(json, sizeof(json),
                 "{\"timestamp_ms\":%lu,\"valid_mask\":%lu,"
                 "\"temperature_centi_c\":%d,\"humidity_centi_pct\":%u,"
                 "\"pressure_pa\":%lu,\"light_raw\":%u,\"mq2_raw\":%u,"
                 "\"mq7_raw\":%u,\"mq135_raw\":%u}",
                 static_cast<unsigned long>(latest_sensor.timestamp_ms),
                 static_cast<unsigned long>(latest_sensor.valid_mask),
                 static_cast<int>(latest_sensor.temperature_centi_c),
                 static_cast<unsigned int>(latest_sensor.humidity_centi_pct),
                 static_cast<unsigned long>(latest_sensor.pressure_pa),
                 static_cast<unsigned int>(latest_sensor.light_raw),
                 static_cast<unsigned int>(latest_sensor.mq2_raw),
                 static_cast<unsigned int>(latest_sensor.mq7_raw),
                 static_cast<unsigned int>(latest_sensor.mq135_raw));
        if (mqtt_client.publish(MQTT_TOPIC_SENSOR, json, false)) {
            sensor_dirty = false;
        }
    }

    if (state_dirty) {
        snprintf(json, sizeof(json),
                 "{\"mode\":%u,\"light_on\":%u,\"door_open\":%u}",
                 static_cast<unsigned int>(latest_state.mode),
                 static_cast<unsigned int>(latest_state.light_on),
                 static_cast<unsigned int>(latest_state.door_open));
        if (mqtt_client.publish(MQTT_TOPIC_STATE, json, true)) {
            state_dirty = false;
        }
    }

    if (ack_dirty) {
        snprintf(json, sizeof(json),
                 "{\"sequence\":%u,\"status\":%u,\"device\":%u,\"value\":%u}",
                 static_cast<unsigned int>(latest_ack.sequence),
                 static_cast<unsigned int>(latest_ack.status),
                 static_cast<unsigned int>(latest_ack.device),
                 static_cast<unsigned int>(latest_ack.value));
        if (mqtt_client.publish(MQTT_TOPIC_ACK, json, false)) {
            ack_dirty = false;
        }
    }
}

/* ========================================
 * 主程序入口
 * ======================================== */

/* ---------- setup() ---------- */
/* 作用：程序启动时执行一次，初始化串口和 WiFi */
void setup()
{
    // 串口配置：256字节接收缓冲区，115200波特率
    Serial.setRxBufferSize(256);
    Serial.begin(115200, SERIAL_8N1);
    Serial.setDebugOutput(false);  // 关闭调试输出

    // WiFi配置：不保存到Flash，STA模式，不自动重连
    WiFi.persistent(false);        // 不保存WiFi配置到Flash
    WiFi.mode(WIFI_STA);           // STA模式（连接路由器）
    WiFi.setAutoReconnect(false);  // 不自动重连（由代码控制）
    WiFi.disconnect();             // 先断开
    if (WIFI_SSID[0] != '\0') {    // 如果配置了WiFi
        startWiFi();               // 开始连接
    }

    mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
    mqtt_client.setCallback(onMqttMessage);
    mqtt_client.setBufferSize(384);
    mqtt_client.setKeepAlive(15);
    mqtt_client.setSocketTimeout(2);
}

/* ---------- loop() ---------- */
/* 作用：主循环，反复执行，处理所有任务 */
void loop()
{
    const unsigned long now = millis();  // 获取当前时间
    processSerial();      // 处理串口数据（接收传感器数据、设备状态等）
    processWiFi(now);     // 处理WiFi重连
    processMqtt(now);     // 连接Broker、接收手机命令、维持MQTT心跳
    publishPendingMqtt(); // 上报STM32送来的传感器、状态和控制结果
    processHeartbeat(now); // 发送STM32串口心跳包
    delay(1);             // 微小延迟，降低CPU功耗
}
