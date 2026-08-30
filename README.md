# 物联网环境控制终端

基于ESP32+FreeRTOS的物联网终端，实现环境传感采集、触摸屏交互、OneNET云端双向控制、网页配网功能。

## 技术栈
- **MCU**: ESP32
- **框架**: ESP-IDF + FreeRTOS
- **显示**: LVGL图形库，240×280触摸屏
- **通信**: MQTT、WebSocket、WiFi
- **云平台**: OneNET
- **外设**: DHT11/DHT22(温湿度)、WS2812(RGB灯带)

## 核心功能
- 3任务+2消息队列架构，解耦传感器采集、LVGL显示、MQTT上报业务
- 接入OneNET云平台，MQTT QoS1发布订阅，JSON物模型上报温湿度与灯光状态
- 云端反向控制WS2812 RGB灯带色彩与亮度
- 移植LVGL驱动240×280触摸屏，实时展示环境数据
- 界面滑块调节WS2812灯光色彩与PWM照明亮度
- AP配网模式：长按按键进入，基于WebSocket实现网页配网，获取WiFi后自动连接
