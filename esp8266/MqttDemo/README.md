# ESP8266最小MQTT示例

这个示例只验证四个动作：连接Broker、订阅Topic、发布消息、在回调中接收Broker转发。它不连接STM32。

## 阅读顺序

```text
setup()
→ mqtt_client.setServer()
→ mqtt_client.setCallback()
→ processMqtt()
→ mqtt_client.connect()
→ mqtt_client.subscribe()
→ mqtt_client.loop()
→ publishTestMessage()
→ mqtt_client.publish()
→ onMqttMessage()
```

因为ESP同时订阅并发布`smarthome/device01/test`，Broker会把ESP发布的测试消息再转发回ESP，所以串口能看到回调输出。

## 使用

1. 复制`mqtt_config.example.h`为`mqtt_config.h`并填写2.4 GHz Wi-Fi。
2. 保证`F:\Smart\tools\mosquitto-lab.conf`对应的Broker正在运行。
3. 将`MQTT_HOST`填写为电脑局域网IP，端口保持1884。
4. 烧录后解除GPIO0接地并复位，使用115200波特率查看日志。

没有实物时可以阅读和编译代码，但不能把MQTT连接结果标记为已验证。
