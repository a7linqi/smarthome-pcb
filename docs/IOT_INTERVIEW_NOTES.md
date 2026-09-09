# 物联网与嵌入式通信复习笔记

> 用途：智能家居项目复盘、校招面试复习和后续 TCP/IP、MQTT、CAN 学习记录。
> 原则：先理解数据怎样流动，再记术语；能结合本项目解释，才算掌握。

## 一、当前项目架构

```text
传感器/按键/执行器
        ↕
STM32F103 + FreeRTOS
        ↕ UART 115200 8N1 + 自定义协议
ESP8266 + Arduino
        ↕ Wi-Fi / TCP / MQTT
MQTT Broker
        ↕
手机端或上位机
```

- STM32：负责传感器采集、本地控制、OLED、按键、蜂鸣器和步进电机。
- ESP8266：负责 Wi-Fi、MQTT、远程命令转发和数据上报。
- STM32 与 ESP8266：通过 USART3/UART0 进行双向通信。

## 二、UART 基础

### 1. UART 有什么特点？

UART 是异步串行通信。异步表示没有单独的时钟线，双方必须约定相同的波特率、数据位、校验位和停止位。

当前项目配置：

```text
115200 bit/s、8 数据位、无校验、1 停止位（8N1）
```

### 2. UART 为什么需要 TX、RX 和 GND？

- TX：发送数据。
- RX：接收数据。
- GND：提供共同的电平参考。
- 两块设备交叉连接：STM32 TX 接 ESP RX，STM32 RX 接 ESP TX。

### 3. UART 是全双工吗？

是。TX 和 RX 是两条独立信号线，理论上可以同时发送和接收。

### 4. 接上线为什么还需要通信协议？

UART 只负责传输连续字节，不知道字节表示温度、命令还是帧头。应用层协议用帧头、类型、长度和 CRC 规定数据边界与含义。

## 三、本项目串口协议

### 1. 帧格式

| 偏移 | 字节数 | 字段 | 说明 |
| ---: | ---: | --- | --- |
| 0 | 1 | SOF0 | 固定 `0xA5` |
| 1 | 1 | SOF1 | 固定 `0x5A` |
| 2 | 1 | Version | 当前为 `0x01` |
| 3 | 1 | Type | 消息类型 |
| 4 | 1 | Sequence | 请求和应答匹配、排序 |
| 5 | 1 | Flags | 当前保留为 0 |
| 6～7 | 2 | Length | Payload 长度，小端序 |
| 8～N | 0～96 | Payload | 业务数据 |
| 末尾 | 2 | CRC16 | CRC-16/CCITT-FALSE，小端序 |

总长度：

```text
完整帧长度 = 8字节协议头 + Payload长度 + 2字节CRC
```

传感器报告的 Payload 是 24 字节，因此完整帧是 `8 + 24 + 2 = 34` 字节。

### 2. Payload 和 Frame 有什么区别？

- Payload：真正的业务数据，例如温度、湿度、设备编号和控制值。
- Frame：在 Payload 外加协议头、长度、序号和 CRC 后形成的完整传输数据。

### 3. 消息类型

| 名称 | 方向 | 用途 |
| --- | --- | --- |
| `HEARTBEAT` | ESP → STM → ESP | 检查 STM32 与 ESP 的串口链路 |
| `SENSOR_REPORT` | STM → ESP | 上报传感器数据 |
| `DEVICE_STATE` | STM → ESP | 上报模式、灯和门状态 |
| `CONTROL_COMMAND` | ESP → STM | 下发设备控制命令 |
| `CONTROL_ACK` | STM → ESP | 返回控制命令执行结果 |

具体数值定义在 `bsp_protocol.h`，工程开发应使用枚举名称，不依靠记忆魔法数字。

### 4. Sequence 有什么作用？

Sequence 是帧序号。发送请求时记录序号，应答沿用相同序号，发送方可以判断应答属于哪一次请求。它还可以辅助去重和排序。

### 5. CRC 能证明什么？

接收方对收到的数据重新计算 CRC，并与帧尾 CRC 比较：

- 相同：传输过程中没有检测到位错误，可以继续处理。
- 不同：数据可能损坏，丢弃该帧。

CRC 不能证明传感器测量正确，也不是加密或身份认证。

### 6. 为什么使用小端序？

协议必须明确多字节数值的排列顺序。当前协议规定低字节先发送，例如：

```text
0x09E8 → E8 09
```

温度以 0.01 ℃ 为单位时，十进制 `2536` 表示 `25.36 ℃`。

### 7. 为什么不用浮点数直接传温度？

使用定点整数可以减少 MCU 间浮点格式、字节序和精度差异，也更方便协议稳定演进。例如 `25.36 ℃` 发送为整数 `2536`。

## 四、STM32 代码分层

| 文件 | 职责 |
| --- | --- |
| `sensor_task.c` | 读取传感器，产生传感器快照 |
| `uart_task.c` | 通信调度：收帧、处理消息、制作业务 Payload、发帧 |
| `bsp_protocol.c` | 通用协议：编码、逐字节解析、长度检查、CRC |
| `bsp_uart_bridge.c` | USART3 字节收发、接收中断和环形缓冲区 |
| `control_task.c` | 执行远程控制命令，生成 ACK 和设备状态 |
| `app_main.c` | 创建任务、队列并启动系统 |

分层的原因：每一层只负责一种问题，便于修改、复用和定位故障。

## 五、STM32 发送链路

```text
Sensor_Task读取传感器
→ uartSensorQueue
→ Uart_Task取出sensor结构体
→ send_sensor_report()
→ 结构体字段写入24字节Payload
→ send_frame()
→ BSP_Protocol_Encode()
→ 添加协议头、长度和CRC，得到34字节Frame
→ BSP_UartBridge_Write()
→ HAL_UART_Transmit()

### 为什么结构体不直接通过串口发送？

结构体可能存在内存对齐、填充字节、编译器差异和字节序差异。显式规定每个字段的偏移和长度，协议才稳定、可读、可跨平台。

### `put_u16_le(payload + 8, value)`是什么意思？

- `payload + 8`等价于`&payload[8]`。
- 从 Payload 的第 8 号位置开始写入。
- `put_u16_le()`把 16 位数拆成低字节和高字节。

## 六、STM32 接收链路

```text
ESP发送字节
→ USART3收到一个字节
→ HAL_UART_RxCpltCallback()
→ 字节写入rx_buf[]环形缓冲区
→ Uart_Task调用BSP_UartBridge_Read()
→ 字节进入rx_data[]
→ BSP_Protocol_Feed()逐字节解析
→ 收齐且CRC正确后得到received_frame
→ handle_frame()
→ 处理心跳或控制命令
```

### 为什么不在串口中断里直接解析协议？

中断应尽快完成。协议解析、CRC、队列操作和回复发送可能耗时，所以中断只保存字节，任务负责复杂处理，避免长时间占用中断上下文。

### `Feed()`为什么一次只接收一个字节？

串口数据可能被分批到达。解析器用内部缓冲区和 `used` 保存进度，逐步寻找帧头、读取长度、等待完整帧并校验 CRC。

未收齐时返回 `BSP_BUSY`；收到完整且有效的帧时返回 `BSP_OK`。

## 七、环形缓冲区

- `head`：下一次写入的位置，由串口接收中断推进。
- `tail`：下一次读取的位置，由 `Uart_Task`推进。
- `head == tail`：缓冲区为空。
- `next == tail`：缓冲区已满。

例子：

```text
初始：head=0，tail=0
接收 A5 5A 01 后：head=3，tail=0
读取三个字节后：head=3，tail=3
```

缓冲区为空只要求 `head == tail`，不要求两个下标回到 0。

当前缓冲区大小为 256，利用：

```c
(index + 1U) & (256U - 1U)
```

实现 `255 → 0` 的回绕。该写法要求数组大小是 2 的幂。

## 八、FreeRTOS 队列

| 队列 | 方向 | 内容 |
| --- | --- | --- |
| `uartSensorQueue` | Sensor_Task → Uart_Task | 等待上报的传感器快照 |
| `remoteControlQueue` | Uart_Task → Control_Task | ESP 下发的控制命令 |
| `controlAckQueue` | Control_Task → Uart_Task | 控制命令执行结果 |
| `deviceStateQueue` | Control_Task → Uart_Task | 当前设备状态 |

`xQueueReceive(queue, &sensor, 0)`把队列项复制到局部结构体变量 `sensor`。
`send_sensor_report(&sensor)`再把该结构体的地址传给函数。队列里保存数据副本，不是简单保存这个局部变量的指针。

## 九、控制命令闭环

```text
ESP发送CONTROL_COMMAND
→ Uart_Task解析为command结构体
→ remoteControlQueue
→ Control_Task判断设备和参数
→ 执行灯、门或模式控制
→ 生成ack结构体
→ controlAckQueue
→ Uart_Task发送CONTROL_ACK
→ ESP根据sequence匹配执行结果
```

`CONTROL_COMMAND` Payload：

```text
payload[0]：设备编号
payload[1]：目标值
```

通信任务不直接操作灯和电机。它把命令交给控制任务，以保持职责清晰并避免阻塞通信处理。

## 十、ESP8266 对应流程

接收：

```text
Serial.read()
→ parser.feed()
→ CRC通过并得到Frame
→ handleFrame()
→ decodeSensorReport()或其他类型处理
```

发送：

```text
准备Payload
→ sendFrame()
→ Protocol::encode()
→ 生成完整协议帧
→ Serial.write()
```

正式使用二进制协议时，ESP8266 UART0 不能混入 `Serial.print()` 调试文本，否则这些文本会进入协议解析器。

## 十一、常见面试问题

### UART、协议和业务处理为什么分层？

UART层负责可靠地搬运本机收到的字节；协议层判断字节是否组成有效帧；业务层根据消息类型完成控制或数据更新。分层可以减少耦合，也方便单独替换通信接口或新增消息。

### 轮询、中断和 DMA 有什么区别？

- 轮询：CPU不断检查状态，实现简单，但浪费CPU时间。
- 中断：事件发生时通知CPU，适合低到中等数据量。
- DMA：外设和内存直接搬运数据，CPU只处理完成事件，适合高速或大量数据。

当前项目接收采用单字节中断加环形缓冲区，发送采用阻塞式 `HAL_UART_Transmit()`。

### CRC失败应该怎么处理？

当前实现丢弃错误帧并重新寻找帧头。更复杂系统还可以记录错误次数、超时、重传或上报告警。

### 心跳检测的是什么？

当前心跳检测的是 STM32 与 ESP8266 之间的串口通信是否正常。Wi-Fi 使用 `WiFi.status()`判断，后续 MQTT 还需要独立判断 Broker 连接和 Keep Alive。

## 十二、排错思路

数据没有到达时按照链路逐层检查：

```text
传感器是否采集成功
→ 队列是否写入/读出
→ Payload字段和长度是否正确
→ Encode是否返回有效帧长
→ USART参数和TX/RX接线是否正确
→ 接收中断是否运行
→ 环形缓冲区是否溢出
→ Feed是否找到帧头并收齐
→ CRC和字节序是否一致
→ Type和Length是否满足业务处理条件
```

## 十三、当前掌握标准

必须能够脱离源码说明：

```text
发送：队列 → 结构体 → Payload → Frame → UART
接收：UART → 环形缓冲区 → Feed → Frame → handle
控制：CONTROL_COMMAND → remoteControlQueue → Control_Task
     → controlAckQueue → CONTROL_ACK
```

暂时不要求默写完整 CRC 和工业级解析器；需要能够根据协议表写出简化发送函数，并能解释、修改和调试现有代码。

## 十四、后续学习区

### TCP/IP（学习中）

#### TCP解决什么问题？

TCP在两台网络设备的应用程序之间提供可靠、有连接、按顺序的字节流。发送方交给TCP的数据即使被底层拆成多个网络包，接收方最终也会按顺序读取；出现丢包时，TCP会进行确认和重传。

“可靠”表示TCP尽力保证数据正确、有序、无重复地交给应用程序，不表示网络永远不断开。应用仍然需要处理连接失败、超时和重连。

#### 数据经过哪些网络层？

本项目发送一条传感器MQTT消息时，可以简化理解为：

```text
应用层：MQTT规定主题、Payload、QoS
传输层：TCP提供端到端可靠字节流和端口
网络层：IP负责跨网络寻址和路由
链路层：Wi-Fi负责当前无线链路上的帧传输
物理层：无线电信号传输比特
```

发送时每层添加自己的控制信息，接收时逐层去掉并交给上一层。应用通常调用网络库，无需自己实现TCP/IP协议栈。

#### IP地址和端口分别是什么？

- IP地址：定位网络中的一台主机或网络接口。
- 端口：定位这台主机上的一个应用服务。

建立TCP连接时通常需要：

```text
服务器域名或IP地址 + 服务器端口
```

例如MQTT客户端连接Broker时，需要知道Broker地址和MQTT服务端口。

#### 客户端和服务器是什么？

- 服务器：监听一个端口，等待连接。
- 客户端：主动连接服务器的IP地址和端口。

本项目中，ESP8266通常是TCP/MQTT客户端，MQTT Broker是服务器。

#### Socket是什么？

Socket是应用程序使用网络协议栈的接口。程序通过Socket完成连接、发送、接收和关闭。ESP8266 Arduino中的`WiFiClient`封装了TCP客户端Socket操作。

典型TCP客户端流程：

```text
连接Wi-Fi并获得IP
→ 解析服务器域名（如果使用域名）
→ 连接服务器IP和端口
→ 发送/接收字节
→ 检测断开
→ 按策略重连
```

一个TCP连接通常可由四元组区分：源IP、源端口、目的IP、目的端口。同一个MQTT Broker可以同时服务许多客户端，因为每条连接的四元组不同。

#### TCP怎样建立和关闭连接？

建立连接使用三次握手：

```text
客户端 → SYN → 服务器
客户端 ← SYN + ACK ← 服务器
客户端 → ACK → 服务器
```

目的包括确认双方收发能力并同步初始序号。不能只背“SYN、SYN-ACK、ACK”，还要知道三次握手完成后才形成可用连接。

正常关闭通常需要双方分别关闭自己的发送方向，因此常见为四次挥手。嵌入式面试第一阶段理解原因即可，暂时不要求默写全部TCP状态。

#### TCP为什么可靠？

核心机制包括：

- 序号：标识字节在数据流中的位置，用于排序和发现缺失。
- ACK确认：接收方确认已经收到的数据范围。
- 超时与重传：规定时间未得到确认时重新发送。
- 校验和：检测TCP报文在传输中的错误。
- 接收窗口：接收方通知发送方自己还能接收多少数据，形成流量控制。
- 拥塞控制：根据网络拥塞情况调整发送速度，避免进一步堵塞网络。

流量控制主要保护接收方，拥塞控制主要保护网络。

`send()`或`client.write()`成功通常只表示数据已经交给本机协议栈或发送缓冲区，不表示对方应用已经处理完成。需要确认业务执行结果时，仍要设计应用层ACK，本项目的`CONTROL_ACK`就是这种确认。

#### 发送成功为什么不等于对端处理成功

以ESP向电脑发送`OPEN_DOOR`为例：

```text
ESP应用程序
client.write("OPEN_DOOR")
        ↓
ESP本地TCP发送缓冲区
        ↓
ESP的TCP协议栈
        ↓
Wi-Fi、路由器和网络
        ↓
电脑的TCP协议栈
        ↓
电脑本地TCP接收缓冲区
        ↓
电脑程序调用recv()
        ↓
电脑程序解析并执行命令
        ↓
电脑程序返回业务ACK
```

| 阶段 | 能证明什么 | 不能证明什么 |
| --- | --- | --- |
| `client.write()`成功 | 数据已交给ESP本地网络库或发送缓冲区 | 不能证明电脑收到 |
| 收到TCP ACK | 对端TCP协议栈确认收到相应字节 | 不能证明对端应用已经读取 |
| 对端`recv()`成功 | 对端应用已经从TCP接收缓冲区取出数据 | 不能证明业务执行成功 |
| 收到业务ACK | 对端应用按照双方约定返回处理结果 | 具体成功与否仍要检查ACK状态码 |

如果ESP本地`client.write()`成功后网络立即断开，数据仍可能没有到达对端。TCP会尝试重传，并最终通过连接错误或超时向应用暴露失败。因此应用必须处理断线和超时。

TCP ACK属于传输层，主要确认字节传输；`CONTROL_ACK`属于应用层，确认具体控制命令的处理结果。两者解决的问题不同，不能互相替代。

本项目UART控制链路也是同一思想：

```text
ESP调用Serial.write()
→ STM32串口中断接收
→ Feed解析并校验CRC
→ handle_frame生成command
→ remoteControlQueue
→ Control_Task执行
→ STM32返回CONTROL_ACK
```

`Serial.write()`成功不能证明STM32已经开门；ESP收到`CONTROL_ACK`并检查状态码后，才能知道命令是成功、参数无效还是设备忙。

#### 如何判断每一层成功到哪一步

工程中应说明“哪一层成功”，不能笼统地说“通信成功”。

| 观察到的信息 | 最多能证明 | 仍不能证明 |
| --- | --- | --- |
| `WiFi.status() == WL_CONNECTED`且获得有效`localIP()` | ESP已连接接入点并获得本地网络配置 | 不能证明能访问互联网、Broker或手机 |
| DNS解析得到Broker IP | 域名能够解析 | 不能证明Broker在线或端口开放 |
| `tcp_client.connect(host, port)`成功 | 当时已与目标IP和端口完成TCP连接建立 | 不能证明MQTT登录成功或业务可用 |
| `client.write()`返回完整长度 | 数据已交给本机TCP发送流程 | 不能证明对端协议栈或应用收到 |
| TCP ACK | 对端TCP协议栈确认收到相应字节 | 不能证明对端应用读取或执行 |
| MQTT收到CONNACK成功码 | Broker接受MQTT连接和认证 | 不能证明指定Topic已经订阅成功 |
| MQTT收到SUBACK | Broker返回订阅结果 | 不能证明未来发布者一定会发布消息 |
| MQTT QoS 0发送调用成功 | 客户端已尝试把消息交给网络层 | 没有Broker级到达确认 |
| MQTT QoS 1收到PUBACK | Broker确认收到该发布消息 | 不能证明手机界面已显示或设备已执行 |
| `HAL_UART_Transmit()`返回`HAL_OK` | STM32串口外设完成了本次发送调用 | 不能证明ESP已正确解析 |
| 接收方CRC通过并生成Frame | 接收到一帧格式和校验正确的数据 | 不能证明业务参数合法或动作执行成功 |
| `xQueueSend()`返回`pdTRUE` | 命令已复制进目标任务队列 | 不能证明目标任务已经执行 |
| 收到`CONTROL_ACK`且状态为OK | STM32业务代码接受了命令并返回成功状态 | 不一定证明真实机械动作已经物理完成 |
| 限位开关、位置传感器或其他反馈确认 | 执行器达到了可测量的目标状态 | 仍需根据系统需求考虑传感器故障 |

当前项目没有门位置限位反馈。门控制中的`CONTROL_ACK OK`表示步进电机运动请求被程序接受，并不严格证明门已经运动到真实目标位置。若简历或面试声称“闭环确认门已打开”，则必须增加限位开关、编码器或其他位置反馈。

判断成功时遵循：

```text
底层成功不能自动推出上层成功；
上层需要自己的状态、应答、超时或物理反馈。
```

#### TCP与UDP有什么区别？

| 对比项 | TCP | UDP |
| --- | --- | --- |
| 连接 | 建立连接后通信 | 无连接 |
| 可靠性 | 确认、重传、排序、去重 | 协议本身不保证 |
| 数据形式 | 连续字节流 | 一条一条数据报，有边界 |
| 开销 | 较大 | 较小 |
| 常见场景 | MQTT、HTTP、文件传输 | DNS、实时音视频、简单广播 |

不能简单说TCP一定比UDP好。需要可靠和有序时常选TCP；更重视实时性、低开销或允许少量丢包时可能选UDP，具体仍由应用需求决定。

#### DHCP、DNS和NAT分别做什么？

- DHCP：ESP接入Wi-Fi后，通常由路由器自动分配IP地址、网关和DNS服务器。
- DNS：把Broker域名解析成IP地址。
- NAT：家用路由器把局域网私有地址转换为公网通信所需的地址信息，使多台内网设备共享公网出口。

一般情况下，ESP主动连接云端Broker能够通过NAT；公网设备想直接主动连接家里的ESP则通常需要额外的端口映射、穿透或由云端Broker转发。

#### TCP和UART有什么相同点与不同点？

共同点：应用层读取到的都可以看成连续字节，单次读取不保证刚好得到一条完整业务消息。

区别：

- UART是两个硬件设备之间的串行字节传输，协议、校验和重发通常由应用自己设计。
- TCP跨网络传输，协议栈提供顺序、确认、重传、流量控制和拥塞控制。
- TCP没有保留应用消息边界。一次`send()`的数据，接收方可能多次`read()`才读完；多次`send()`的数据也可能一次`read()`读到。

#### 什么是TCP粘包和拆包？

TCP提供字节流，不提供“消息一条一条到达”的保证：

- 拆包：一次发送的应用消息需要多次读取才能收齐。
- 粘包：多次发送的应用消息在一次读取中一起出现。

常见解决方式：固定长度、分隔符、长度字段或成熟的上层协议。MQTT已经定义自己的报文格式和剩余长度字段，使用MQTT库时通常由库处理报文边界。

#### TCP与MQTT是什么关系？

TCP负责在客户端与Broker之间可靠传输字节；MQTT在TCP之上规定连接、发布、订阅、主题、QoS和心跳等应用层含义。

```text
Wi-Fi：让ESP接入局域网
IP：寻找目标主机
TCP：建立可靠字节连接
MQTT：规定物联网消息怎样发布和订阅
```

TCP Keepalive、MQTT Keep Alive和本项目UART心跳不是同一个机制：

- TCP Keepalive：操作系统或协议栈检查TCP对端是否仍然可达，参数通常较底层。
- MQTT Keep Alive：客户端与Broker通过MQTT控制报文维持会话并检测失联。
- UART心跳：本项目ESP与STM32之间自定义的链路检测。

实际项目需要分别管理Wi-Fi状态、TCP/MQTT连接状态和STM32串口链路状态。

#### 当前阶段需要掌握到什么程度？

必须掌握IP、端口、客户端/服务器、连接、字节流、断开重连、TCP与UDP的主要差异，以及TCP为何存在粘包拆包。暂时不要求实现TCP协议栈，也不要求背诵TCP状态机的全部状态。

配套源码见 `esp8266/TcpClientDemo`：ESP作为TCP客户端连接电脑端回显服务器，定期发送文本并读取回复。

### MQTT

#### 发布订阅模型

- Client：连接Broker的设备或程序；ESP和手机都是Client。
- Broker：接受客户端连接，根据Topic和订阅关系转发消息。
- Topic：消息分类和路由路径。
- Payload：具体消息内容，本质是一组字节，可以使用JSON表达业务字段。
- Publish：客户端向某个Topic发布消息。
- Subscribe：客户端订阅Topic过滤器并接收匹配消息。

```text
传感器：ESP发布 → Broker转发 → 手机订阅接收
控制命令：手机发布 → Broker转发 → ESP订阅接收
```

Broker默认主要用于转发消息，不是历史数据库。历史传感器数据需要由后端或其他订阅者写入数据库。

#### QoS

| QoS | 语义 | 过程和特点 | 本项目建议 |
| ---: | --- | --- | --- |
| 0 | 最多一次 | 不等待MQTT确认，开销低，消息可能丢失 | 周期传感器数据 |
| 1 | 至少一次 | 收到PUBACK才确认，超时可重发，因此可能重复 | 控制命令、ACK、设备状态 |
| 2 | 恰好一次 | PUBLISH/PUBREC/PUBREL/PUBCOMP，开销最大 | 当前项目不需要 |

QoS 1可能重复，所以控制命令应使用目标状态并支持去重或幂等。例如“设置灯=1”重复执行仍然是开灯；“翻转灯”重复执行可能得到错误状态。

MQTT PUBACK表示Broker确认收到QoS 1发布消息，不代表STM32已经执行。STM32业务执行结果仍由`CONTROL_ACK`确认。

#### Retain保留消息

发布时设置Retain后，Broker为该Topic保存最新一条保留消息。新的订阅者订阅后，可以立即收到该消息，不必等待设备下一次发布。

适合保留：设备当前状态、在线状态、必要时的最新传感器快照。不适合把Retain当历史数据库；每个Topic通常只保留最新一条保留消息。

本项目示例：

```text
Topic: smarthome/device01/state
Payload: {"light":1,"door":0,"mode":0}
QoS: 1
Retain: true
```

手机刚打开并订阅`state`时，Broker立即发送最新状态。

#### Keep Alive

MQTT Keep Alive用于检测客户端与Broker之间的MQTT连接是否仍然有效。在没有其他MQTT控制报文时，客户端发送`PINGREQ`，Broker回复`PINGRESP`。

ESP程序必须频繁调用MQTT库的网络处理函数（例如常见库中的`client.loop()`），让它接收消息、处理确认和维持Keep Alive。长时间阻塞可能造成MQTT掉线。

Keep Alive不是UART心跳，也不是单纯的`WiFi.status()`。Wi-Fi仍连接时，Broker连接也可能已经断开。

#### Last Will遗嘱消息

客户端连接Broker时预先设置遗嘱Topic和Payload。如果客户端异常断电、网络消失或未正常发送DISCONNECT，Broker会替客户端发布遗嘱消息。客户端正常断开时通常不发布遗嘱。

推荐在线状态方案：

```text
连接前设置遗嘱：status = offline，Retain = true
MQTT连接成功后发布：status = online，Retain = true
ESP异常掉线后Broker发布：status = offline
```

这样手机新订阅`status`时可以立即得到Broker保存的最新在线状态。

#### 下一步

#### 本项目Topic设计

设备根路径暂定为：

```text
smarthome/device01
```

| Topic | 方向 | QoS | Retain | 用途 |
| --- | --- | ---: | --- | --- |
| `smarthome/device01/sensor` | ESP → 手机/后端 | 0 | false | 周期传感器数据 |
| `smarthome/device01/state` | ESP → 手机/后端 | 0 | true | 当前模式、灯和门状态 |
| `smarthome/device01/command` | 手机/后端 → ESP | 1 | false | 远程控制命令 |
| `smarthome/device01/ack` | ESP → 手机/后端 | 0 | false | 控制命令处理结果 |
| `smarthome/device01/status` | ESP/Broker → 手机/后端 | 普通上线0，遗嘱1 | true | online/offline |

当前选择的 PubSubClient 2.8 普通 `publish()` 只提供 QoS 0；它支持 QoS 1订阅和
QoS 1遗嘱。表中是当前代码真实能力，不要把期望设计写成已经实现。业务控制结果仍由
STM32的`CONTROL_ACK`确认。

控制命令禁止设置Retain，否则ESP重连并重新订阅后，可能收到并再次执行旧命令。状态和在线信息适合Retain，因为新订阅者需要立即得到当前值。

#### 本项目JSON Payload初稿

传感器数据：

```json
{"uptime_ms":123456,"valid_mask":7,"temperature_c":25.36,"humidity_pct":60.12,"pressure_pa":101325,"light_raw":1800,"mq2_raw":350,"mq7_raw":280,"mq135_raw":420}
```

设备状态：

```json
{"mode":0,"light":1,"door":0}
```

控制命令：

```json
{"device":"light","value":1}
```

控制结果：

```json
{"status":"ok","device":"light","value":1,"sequence":7}
```

在线状态：

```text
online
offline
```

MQTT层使用可读字符串和JSON；ESP收到命令后，将`light/door/mode`转换为UART协议中的设备枚举，将JSON数值转换为`CONTROL_COMMAND` Payload。UART层仍使用紧凑二进制协议。

后续可增加`request_id`实现手机请求与ACK的严格匹配。第一版先使用UART `sequence`观察命令链路，避免一次引入过多状态管理。

已完成软件实现：本地Mosquitto、ESP发布订阅与断线重连，以及MQTT命令接入UART桥接。
待实物验证：Wi-Fi、UART电气连接、真实传感器、实际设备动作和长时间稳定性。

### CAN

待补充：物理层、差分信号、帧结构、标识符、仲裁、位时序、过滤器、中断收发和错误状态。

## 十五、自测题

1. 为什么 UART 接上线以后仍然需要应用层协议？
2. Payload 与完整 Frame 有什么区别？
3. 为什么24字节传感器 Payload 会形成34字节完整帧？
4. CRC通过能否说明温度测量一定正确？
5. 为什么多字节数据必须约定字节序？
6. 为什么串口中断只存字节，不直接处理控制命令？
7. 环形缓冲区中 `head`、`tail`分别由谁改变？
8. `head == tail`和`next == tail`分别表示什么？
9. 四个FreeRTOS队列分别连接哪些任务？
10. ESP要求开门后，命令和ACK如何在系统中流动？
