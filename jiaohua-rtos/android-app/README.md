# 智能浇花 Android APP

该工程把 `../mobile-dashboard` 中的控制界面打包进 APK。APP启动后直接通过
MQTT over WebSocket连接设备，不需要电脑运行网页服务器，也不依赖GitHub Pages。

## 自动构建

推送Android工程或手机界面的改动后，GitHub Actions中的`Build Android APK`
工作流会生成调试安装包。打开该次工作流，在Artifacts中下载
`JiaoHua-debug-apk`，解压后安装`app-debug.apk`。

调试APK适合项目测试。正式发布前需要创建并妥善保管独立的签名密钥。
