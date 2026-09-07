# ESP8266 常用 API 与智能家居应用

适用：ESP8266 + Arduino Core 3.1.2。按“已经学过 STM32 HAL”的基础编写。

这是一份查阅笔记，不要求一次背完。先学串口和时间，再学 Wi-Fi，最后接入 MQTT。
文中的短代码块是独立用法片段，不能全部拼进一个工程；完整联网例程在文末给出项目路径。

## 1. 先对应你熟悉的 STM32 思路

| 你要做什么 | STM32 中的做法 | ESP8266 Arduino 中的做法 |
|---|---|---|
| 上电初始化 | main 中调用 MX_xxx_Init | setup 中调用初始化 API |
| 持续处理业务 | while(1) 或任务函数 | loop 被框架反复调用 |
| 配置 GPIO | HAL_GPIO_Init | pinMode |
| 设置输出 | HAL_GPIO_WritePin | digitalWrite |
| 读取输入 | HAL_GPIO_ReadPin | digitalRead |
| 毫秒时间 | HAL_GetTick | millis |
| 串口发送 | HAL_UART_Transmit | Serial.write |
| 从接收队列取字节 | 自己的环形缓冲区接口 | Serial.available、Serial.read |
| 联网 | MCU 与网络模块配合 | WiFi 对象提供接口 |

这是功能对照，不能认为执行方式、阻塞行为完全相同。ESP 这里不需要 CubeMX，也不用照搬 STM32 FreeRTOS 的任务创建方式。

```cpp
void setup() {
    // 上电或复位后执行一次：初始化。
}

void loop() {
    // 每次尽快处理完，再返回；框架会再次调用。
}
```

`Serial`、`WiFi` 是 C++ 对象，点号表示调用它的成员函数。先会用即可，不必先学完整的 C++。

## 2. GPIO：会查就行，Smart 的执行器主要由 STM32 控制

| 用法 | 参数和结果 | 应用 |
|---|---|---|
| pinMode(pin, OUTPUT) | pin 是 GPIO 编号 | 设置输出 |
| pinMode(pin, INPUT) | 设置输入 | 读外部数字信号 |
| pinMode(pin, INPUT_PULLUP) | 输入并启用内部上拉 | 配合对地按键 |
| digitalWrite(pin, HIGH/LOW) | 设置高/低电平 | 指示灯 |
| digitalRead(pin) | 返回 HIGH 或 LOW | 检测输入 |

例如 `digitalWrite(2, LOW)` 操作 GPIO2，不是排针第二根。

```cpp
// 仅当你确认自己的板载 LED 接在 GPIO2、低电平点亮时使用。
void setup() {
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
}
void loop() {}
```

GPIO0 是启动配置脚。正常运行与串口下载所需电平不同，不要为了练按键随意改变启动接线。
模块引出的脚比芯片总引脚少，不是所有 API 涉及的引脚都能在 ESP-01/01S 上找到。

## 3. 串口：当前项目最需要掌握的一组

| 用法 | 含义／返回值 | 常见用途 |
|---|---|---|
| Serial.begin(115200) | 初始化，默认 8N1 | 与 STM32 波特率一致 |
| Serial.available() | 当前可读取的字节数 | 先判断是否有数据 |
| Serial.read() | 返回一个字节 0～255；无数据返回 -1 | 逐字节解析 |
| Serial.peek() | 看下一个字节，但不取走 | 预读 |
| Serial.write(byte) | 发送原始字节，返回写入字节数 | 发送协议 |
| Serial.write(buf, len) | 发送指定长度的原始数据 | 一次发送一帧 |
| Serial.print(value) | 输出可读文本 | 调试 |
| Serial.println(value) | 输出文本并附加换行 | 调试日志 |
| Serial.flush() | 等待发送完成 | 不是清空接收缓冲区 |

### print 和 write 的区别

```cpp
Serial.print(65);          // 发送字符 '6'、'5'，两个字节
Serial.write((uint8_t)65); // 发送数值 65，即 0x41，一个字节
```

传二进制协议用 write；给人看的日志用 print/println。

### 应用 A：串口回显（完整小程序）

```cpp
void setup() {
    Serial.begin(115200);
}

void loop() {
    // 限制单次处理量，持续来数据时也能返回，留时间给其他工作。
    for (int count = 0; count < 64 && Serial.available() > 0; ++count) {
        int value = Serial.read();
        if (value >= 0) {
            Serial.write((uint8_t)value);
        }
    }
    delay(1);
}
```

验证：USB-TTL 接收端连接 ESP TX，发送端连接 ESP RX，双方共地；发送 ABC，收到 ABC。
不要同时让 USB-TTL TX 和 STM32 TX 驱动同一根 ESP RX。

### 应用 B：读取一帧的正确思路

`available() > 0` 只说明至少有一个字节，不代表一整帧都到了！

```text
读一个字节 → 找帧头 → 收帧头剩余字段 → 检查长度
           → 收负载和 CRC → 校验 → 再交给业务代码
```

Smart 已约定 A5 5A 帧头、版本、类型、序号、长度及 CRC16。以后直接按协议逐字节解析，不能把二进制数据当字符串。
Serial 本身有接收缓冲，但缓冲区有限；长时间不读仍可能丢数据。
`write()` 在发送缓冲满时也会等待，不等于任何时候都完全非阻塞。

### Smart 的实际串口分工

```text
STM32 PB10 / USART3_TX → ESP RX / GPIO3
STM32 PB11 / USART3_RX ← ESP TX / GPIO1
STM32 GND             — ESP GND
```

这是 Smart 的现有代码和引脚表，接线前以智能家居板实物核对。不要套用另一个浇花板的 PA2/PA3。
正式桥接时不要在同一条协议串口随意 println 调试文字。Serial1 只有发送能力，不能当第二个完整收发串口。

## 4. 时间：把 HAL_GetTick 的思路搬过来

| API | 单位 | 用途 |
|---|---|---|
| millis() | 毫秒 | 周期任务、超时、重连间隔 |
| micros() | 微秒 | 测量较短时间 |
| delay(ms) | 毫秒 | 暂停当前程序流程，同时给网络后台运行机会 |
| delayMicroseconds(us) | 微秒 | 短延时，不适合长时间等待网络 |
| yield() | 无 | 主动让后台工作有机会执行 |

### 应用：每秒处理一次，同时继续读串口

```cpp
unsigned long last_ms = 0;

void setup() {
    Serial.begin(115200);
}

void loop() {
    unsigned long now = millis();
    if (now - last_ms >= 1000UL) {
        last_ms = now;
        Serial.println("one second"); // 仅用于调试练习
    }
    // 在这里还可以检查串口、Wi-Fi，而不是 delay(1000)。
    delay(1);
}
```

用无符号的 `now - last_ms` 计算经过时间，可处理计数回绕；这种写法也适用于 STM32。
这种定时是轮询调度，不是精密硬件定时器。长耗时函数仍会推迟下一次执行。

## 5. Wi-Fi：学会“发起操作”和“查询结果”分开

先添加：

```cpp
#include <ESP8266WiFi.h>
```

| API | 用途 | 例子 |
|---|---|---|
| WiFi.mode(WIFI_STA) | 客户端模式，连接路由器 | 智能家居常用 |
| WiFi.begin(ssid, password) | 发起连接，不表示已经拿到 IP | WiFi.begin("MyWiFi", "password") |
| WiFi.status() | 查询状态，返回 wl_status_t | 与 WL_CONNECTED 比较 |
| WiFi.isConnected() | 返回是否已连接 | 用于简单条件判断 |
| WiFi.localIP() | 获取 IPAddress 对象 | Serial.println(WiFi.localIP()) |
| WiFi.RSSI() | 信号强度，单位 dBm | 连接后查看 |
| WiFi.disconnect() | 断开连接 | 重新开始连接流程 |
| WiFi.reconnect() | 尝试重新连接 | 返回成功发起不等于已经联网 |
| WiFi.setAutoReconnect(true) | 启用库的自动重连 | 不代替第一次 begin |
| WiFi.persistent(false) | 后续配置不反复保存到 Flash | 重试前配置一次 |
| WiFi.scanNetworks() | 扫描并返回网络数量或错误结果 | 调试周围网络，默认扫描会等待 |
| WiFi.SSID(i) | 读取第 i 个扫描结果名称 | i 从 0 开始 |
| WiFi.scanDelete() | 释放扫描结果 | 扫描输出完成后调用 |

常见状态符号：`WL_CONNECTED` 已连接、`WL_NO_SSID_AVAIL` 没找到目标网络、
`WL_CONNECT_FAILED` 连接失败、`WL_DISCONNECTED` 未连接。不要只凭一个状态就断言是焊接或密码问题。

### 应用：最小连接与超时重试（完整小程序）

```cpp
#include <ESP8266WiFi.h>

const char* ssid = "替换为2.4GHz网络名称";
const char* password = "替换为密码";
unsigned long attempt_ms = 0;
bool was_connected = false;

void setup() {
    Serial.begin(115200);
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false); // 本示例由 loop 统一重试
    WiFi.begin(ssid, password);
    attempt_ms = millis();
}

void loop() {
    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !was_connected) {
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }
    if (!connected && was_connected) {
        Serial.println("WiFi lost");
        attempt_ms = millis();
    }
    was_connected = connected;

    if (!connected && millis() - attempt_ms >= 20000UL) {
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        attempt_ms = millis();
    }
    // 后续可在这里处理串口，不必一直等 Wi-Fi 连上。
    delay(1);
}
```

练习：连接成功后关掉热点，再打开，观察是否再次输出 IP。
这里没有无限等待连接的 while，但不能因此认为每个库函数都零耗时。
拿到 IP 只证明接入局域网，不证明 MQTT 服务器已连接，也不保证互联网畅通。

## 6. 其他接口：知道能做什么，暂时不用全学

| 功能 | 接口 | 本项目建议 |
|---|---|---|
| ADC | analogRead(A0) | 由 STM32 采集；ESP-01/01S 通常未引出 A0 |
| PWM | analogWrite(pin, value) | 了解即可，执行器由 STM32 控制 |
| I2C | Wire.begin、beginTransmission、write、endTransmission、requestFrom | 先不在 ESP 上重复移植传感器 |
| 软件重启 | ESP.restart() | 故障恢复辅助，不要用无限重启代替排错 |
| 可用堆内存 | ESP.getFreeHeap() | 长时间运行时观察内存变化 |

ADC 裸芯片量程不能直接当 3.3V；PWM 在 Core 3.x 默认范围为 0～255，与某些旧教程不同。
涉及接线和电压时先核对模块资料。上述接口是扩展索引，不是要求你立即操作硬件。

## 7. MQTT 是下一层，不是 WiFi.begin 自动完成的

```text
传感器 → STM32 → UART → ESP8266 → MQTT 服务器 → 手机
设备   ← STM32 ← UART ← ESP8266 ← MQTT 服务器 ← 手机命令
```

后续选定 MQTT 客户端库后，再学习该库的连接、发布、订阅、收消息回调和维持连接接口。
Wi-Fi 库和 MQTT 库不是一回事，不要把网上不同库的同名函数混用。
远程控制消息必须经过检查，再发给 STM32；收到命令不等于执行成功，还需要 STM32 应答。

## 8. 当前推荐练习顺序

1. 串口回显：弄清 write 与 print 的区别。
2. millis 定时：不使用长 delay，也能定期处理事情。
3. Wi-Fi 连接：读状态和 IP，测试错误密码与断网恢复。
4. UART 帧解析：处理分段接收、CRC 错误和重新寻找帧头。
5. MQTT：先上报一个固定数值，再接真实传感器，最后接控制命令。

每学一个 API，确认四件事：输入是什么、返回什么、是否等待、失败后怎么处理。

## 9. 与当前工程对应

- 已有联网程序：[WiFiConnect.ino](/F:/Smart/esp8266/WiFiConnect/WiFiConnect.ino)。
- 正确路径：`F:\Smart\esp8266\WiFiConnect\WiFiConnect.ino`。
- 凭据配置：`F:\Smart\esp8266\WiFiConnect\wifi_config.h`，密码只在本机填写。
- 协议文档：`F:\Smart\docs\UART_PROTOCOL.md`。
- STM32 串口底层：`F:\Smart\stm32\SmartHome\BSP\Src\bsp_uart_bridge.c`。
- 项目进度：`F:\Smart\docs\PROJECT_PROGRESS.md`。

现有 WiFiConnect 项目已用 Core 3.1.2 编译通过；本笔记中的教学示例未逐个上板验证。

## 10. 查 API 的入口

- [ESP8266 Core 3.1.2：GPIO、时间、串口参考](https://arduino-esp8266.readthedocs.io/en/3.1.2/reference.html)
- [ESP8266 Core 3.1.2：Wi-Fi Station API](https://arduino-esp8266.readthedocs.io/en/3.1.2/esp8266wifi/station-class.html)
- [Arduino Serial.print 参考](https://docs.arduino.cc/language-reference/en/functions/communication/serial/print/)

查资料时加上“ESP8266 Arduino 3.1.2”，避免误读 ESP32/ESP-IDF 教程。
也可以在 Arduino IDE 中查看示例，或跳转到库的头文件查看参数与返回类型。
