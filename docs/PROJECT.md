# WeatherStation e-paper remote

> 最后核对日期：2026-08-10

## 项目概览

这是一个运行在 ESP8266 NodeMCU v2 上的低功耗墨水屏天气站固件。设备通过 Wi-Fi 获取和风天气数据与网络时间，在支持的墨水屏上显示天气、空气质量和时间，并通过深睡降低功耗。

## 已确认的实现

- 主程序入口：`WeatherStation-epaper-remote.ino`。
- `TimeClient.cpp` 负责 NTP、HTTP Date 解析、时区换算以及跨深睡的软件时钟推进。
- `heweather.cpp` 负责和风天气网络请求与响应解析。
- `EPD_drive.cpp`、`EPD_drive_gpio.cpp` 负责多个墨水屏型号的初始化、刷新和休眠。
- 配置保存在 LittleFS 与 EEPROM 缓存中；时间、刷新阶段和退避状态使用 ESP8266 RTC 用户内存跨深睡保存。
- WF32（3.2 英寸）在显示时间时采用局刷、短深睡、屏幕关断、再睡至下一分钟的两阶段流程。

## 开发与验证

| 用途 | 方法 |
|---|---|
| 构建 | Arduino IDE 1.8.9 / ESP8266 core 3.1.2；复用 `.build/build.options.json` 中的 NodeMCU v2 FQBN 与库路径 |
| 测试 | `tests/epd_raster_tests.cpp` 使用旧逐点算法作为 oracle，对 XBM 帧缓冲合成做宿主端逐字节差分验证；修改固件后仍需完整编译 |
| 硬件验证 | 烧录连接的 ESP8266，观察启动串口、对应屏幕型号的局刷/全刷路径和网络校时后的显示 |

## 关键约束与风险

- ESP8266 深睡期间无法用 `millis()` 直接测量真实睡眠时长，软件时钟依赖计划睡眠时长并会产生漂移，因此联网周期必须重新校时。
- 屏幕 BUSY、刷新完成和休眠命令属于高功耗敏感路径，故障处理必须保持有界等待并尽量关闭高压电路。
- 设备配置、凭据和生成的固件不应提交到仓库。
