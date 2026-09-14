# 智能浇花手机控制端

这是一个适配手机屏幕的 MQTT Web 控制端，可添加到手机主屏幕作为 PWA 使用。

## 本地预览

不要直接双击 `index.html`。在本目录启动静态 Web 服务后，通过浏览器访问页面，Service Worker和MQTT WebSocket才能正常工作。

## MQTT配置

- Broker：`wss://broker.emqx.io:8084/mqtt`
- 数据主题：`jiaohua/<设备编号>/data`
- 控制主题：`jiaohua/<设备编号>/control`
- 应答主题：`jiaohua/<设备编号>/ack`

页面右上角的设置按钮可以修改 Broker地址和设备编号，设置保存在浏览器本地。

公共 Broker仅用于项目开发和演示，控制命令不使用retain，避免设备重新上线后执行旧命令。
