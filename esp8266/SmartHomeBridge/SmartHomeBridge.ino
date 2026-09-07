#include <ESP8266WiFi.h>

#include "bridge_config.h"
#include "protocol.h"

/*
 * ESP8266 UART0：GPIO1=TX，GPIO3=RX，115200 8N1。
 * 正式运行时 Serial 全部用于二进制协议，不能混入 println 调试文字。
 */

/* ---------- 协议相关变量 ---------- */
static Protocol::Parser parser;           // 帧解析器，逐字节组装完整帧
static Protocol::Frame rx_frame;          // 接收缓冲区，存储解析好的帧
static uint8_t tx_buffer[Protocol::MAX_FRAME]; // 发送缓冲区
static uint8_t next_sequence = 0;         // 帧序号，每发一帧+1，用于匹配请求和响应

/* ---------- WiFi 和心跳相关变量 ---------- */
static unsigned long wifi_attempt_ms = 0;       // 上次尝试连接WiFi的时间
static unsigned long heartbeat_ms = 0;          // 上次发送心跳的时间
static unsigned long last_heartbeat_ack_ms = 0; // 上次收到心跳回复的时间
static uint8_t pending_heartbeat_sequence = 0;  // 等待回复的心跳帧序号
static bool waiting_heartbeat_ack = false;       // 是否正在等待心跳回复

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

/* ---------- 发送帧函数 ---------- */
static void sendFrame(uint8_t type, uint8_t sequence,
                      const uint8_t *payload, uint16_t length)
{
    const size_t count = Protocol::encode(type, sequence, payload, length,
                                           tx_buffer, sizeof(tx_buffer));
    if (count > 0) {
        Serial.write(tx_buffer, count);  // 通过串口发送编码后的帧
    }
}

/* ---------- 发送控制命令 ---------- */
/* MQTT 收到控制消息后将调用此函数；目前先保留为串口发送入口。 */
static void sendControlCommand(uint8_t device, uint8_t value)
{
    const uint8_t payload[2] = {device, value};
    sendFrame(Protocol::CONTROL_COMMAND, next_sequence++, payload, sizeof(payload));
}

/* ---------- 解码传感器报告 ---------- */
static void decodeSensorReport(const Protocol::Frame &frame)
{
    if (frame.length != 24) {
        return;  // 数据长度不对，丢弃
    }
    // 按偏移量解析各个字段（小端字节序）
    latest_sensor.timestamp_ms = Protocol::getU32LE(frame.payload + 0);
    latest_sensor.valid_mask = Protocol::getU32LE(frame.payload + 4);
    latest_sensor.temperature_centi_c =
        static_cast<int16_t>(Protocol::getU16LE(frame.payload + 8));
    latest_sensor.humidity_centi_pct = Protocol::getU16LE(frame.payload + 10);
    latest_sensor.pressure_pa = Protocol::getU32LE(frame.payload + 12);
    latest_sensor.light_raw = Protocol::getU16LE(frame.payload + 16);
    latest_sensor.mq2_raw = Protocol::getU16LE(frame.payload + 18);
    latest_sensor.mq7_raw = Protocol::getU16LE(frame.payload + 20);
    latest_sensor.mq135_raw = Protocol::getU16LE(frame.payload + 22);
    latest_sensor.received = true;  // 标记已收到数据
}

/* ---------- 处理收到的帧 ---------- */
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
    }
    else if (frame.type == Protocol::CONTROL_ACK && frame.length == 3) {
        // 收到控制命令的应答（STM32 执行完命令后回复）
        // payload：[执行状态][设备编号][实际值]
        const uint8_t status = frame.payload[0];    // 执行状态
        const uint8_t device = frame.payload[1];    // 设备编号
        const uint8_t value = frame.payload[2];     // 实际值
        (void)status;   // 暂时不用，避免编译警告
        (void)device;   // 暂时不用
        (void)value;    // 暂时不用
        // 下一阶段交给 MQTT 上报到服务器
    }
}

/* ---------- 处理串口数据 ---------- */
static void processSerial()
{
    /* 每轮限制处理量（最多128字节），让 Wi-Fi 后台和其他逻辑也能获得运行时间。 */
    for (uint16_t count = 0; count < 128 && Serial.available() > 0; ++count) {
        const int value = Serial.read();
        // 逐字节喂给解析器，收齐一帧后处理
        if (value >= 0 && parser.feed(static_cast<uint8_t>(value), rx_frame)) {
            handleFrame(rx_frame);
        }
    }
}

/* ---------- 处理心跳 ---------- */
static void processHeartbeat(unsigned long now)
{
    // 每2秒发送一次心跳
    if (now - heartbeat_ms < 2000UL) {
        return;
    }
    heartbeat_ms = now;

    // 心跳负载：当前时间 + WiFi连接状态
    uint8_t payload[5];
    Protocol::putU32LE(payload, now);
    payload[4] = WiFi.status() == WL_CONNECTED ? 1 : 0;
    pending_heartbeat_sequence = next_sequence++;
    waiting_heartbeat_ack = true;
    sendFrame(Protocol::HEARTBEAT, pending_heartbeat_sequence,
              payload, sizeof(payload));
}

/* ---------- 启动WiFi连接 ---------- */
static void startWiFi()
{
    wifi_attempt_ms = millis();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

/* ---------- 处理WiFi重连 ---------- */
static void processWiFi(unsigned long now)
{
    // 如果没配置WiFi或已连接，直接返回
    if (WIFI_SSID[0] == '\0' || WiFi.status() == WL_CONNECTED) {
        return;
    }
    // 断开超过20秒自动重连
    if (now - wifi_attempt_ms >= 20000UL) {
        WiFi.disconnect();
        startWiFi();
    }
}

void setup()
{
    // 串口配置：256字节接收缓冲区，115200波特率
    Serial.setRxBufferSize(256);
    Serial.begin(115200, SERIAL_8N1);
    Serial.setDebugOutput(false);

    // WiFi配置：不保存到Flash，STA模式，不自动重连
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect();
    if (WIFI_SSID[0] != '\0') {
        startWiFi();
    }
}

void loop()
{
    const unsigned long now = millis();
    processSerial();      // 处理串口数据（接收传感器数据、设备状态等）
    processHeartbeat(now); // 发送心跳包
    processWiFi(now);     // 处理WiFi重连
    delay(1);             // 微小延迟，降低CPU功耗
}
