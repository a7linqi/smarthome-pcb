# ESP8266 TCP客户端学习示例

该示例只学习 Wi-Fi、IP、端口和 TCP 客户端收发，不包含 MQTT，也不连接 STM32。

## 文件

- `TcpClientDemo.ino`：ESP8266 TCP客户端。
- `tcp_config.example.h`：Wi-Fi、电脑IP和端口配置模板。
- `tcp_echo_server.py`：在电脑上运行的TCP回显服务器。

## 运行步骤

1. 复制 `tcp_config.example.h` 为 `tcp_config.h`。
2. 在 `tcp_config.h` 填写2.4 GHz Wi-Fi名称、密码和电脑的局域网IPv4地址。
3. 确保电脑与ESP8266连接同一个局域网。
4. 在本目录运行 `python tcp_echo_server.py`，监听端口5000。
5. 编译并烧录 `TcpClientDemo.ino`，解除GPIO0接地后复位。
6. 使用USB-TTL在115200波特率观察ESP日志和服务器回显。

如果Windows防火墙询问权限，只允许专用网络即可。测试时ESP8266 UART0用于USB-TTL日志，不要让USB-TTL TX和STM32 TX同时驱动ESP RX。

## 代码与理论对应

```text
WiFi.begin()                 连接无线局域网
WiFi.localIP()               查看DHCP分配的ESP本机IP
tcp_client.connect(host,port) 连接服务器IP和端口
tcp_client.print()/println()  向TCP字节流写数据
tcp_client.available()       查询本地接收缓冲区已有多少字节
tcp_client.read()            从TCP字节流读取一个字节
tcp_client.connected()       查询客户端当前记录的连接状态
tcp_client.stop()            关闭连接并释放资源
```

`print()`调用成功不表示服务器业务已经处理。服务器收到数据后主动返回`ECHO:`，这个回复才是本示例的应用层确认。
