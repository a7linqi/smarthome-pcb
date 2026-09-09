# 本地MQTT实验

## 组件

- Broker：`D:\Mosquitto\mosquitto.exe`
- 发布工具：`D:\Mosquitto\mosquitto_pub.exe`
- 订阅工具：`D:\Mosquitto\mosquitto_sub.exe`
- 项目实验端口：`1884`
- 配置文件：`F:\Smart\tools\mosquitto-lab.conf`

系统默认Mosquitto服务监听`127.0.0.1:1883`。为避免修改系统服务，项目实验实例使用`0.0.0.0:1884`。

当前自动测试只证明电脑本机Broker可用。ESP8266上板并取得IP后，需要在Windows防火墙
中把TCP 1884入站范围限制为该ESP的IP，再进行局域网测试；不要为整个公共网络开放
匿名Broker。

## 手动启动Broker

```powershell
& 'D:\Mosquitto\mosquitto.exe' -c 'F:\Smart\tools\mosquitto-lab.conf' -v
```

## 订阅测试Topic

另开一个终端：

```powershell
& 'D:\Mosquitto\mosquitto_sub.exe' -h 127.0.0.1 -p 1884 -t 'smarthome/device01/test' -v
```

## 发布测试消息

再开一个终端：

```powershell
& 'D:\Mosquitto\mosquitto_pub.exe' -h 127.0.0.1 -p 1884 -t 'smarthome/device01/test' -m 'hello mqtt'
```

订阅终端应显示：

```text
smarthome/device01/test hello mqtt
```

该配置允许匿名连接，只适用于受信任的学习局域网。正式部署需要添加账号认证、访问控制和TLS。

## 自动检查Topic规则

Broker启动后执行：

```powershell
& 'F:\Smart\tools\test_mqtt_topics.ps1'
```

脚本检查两条关键规则：设备状态使用Retain，新订阅者能立即拿到；控制命令不使用
Retain，后来上线的设备不会误执行旧命令。出现`PASS`才算电脑端规则测试成功。
