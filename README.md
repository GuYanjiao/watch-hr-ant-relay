# Watch HR ANT Relay

目标：nRF52840 接收手表的 BLE 心率数据，按 ANT+ 心率设备规范发送。

开发板：Pro Micro 外形 nRF52840 红板。编辑器：VS Code。

## 开发环境

- 源码在当前目录，Git 本地仓库，沿用用户现有提交身份。
- Nordic 工具安装在 C:\ncs，避免把 SDK 和编译器放入 Git。
- 版本基线：nRF Connect SDK v3.2.4，ANT Add-on v2.1.1。
- ANT 与 SDK 版本依照官方兼容表固定：https://ant-nrfconnect.github.io/doc/compatibility.html
- 官方 ANT 安装指南：https://ant-nrfconnect.github.io/doc/getting_started.html

当前阶段配置环境，尚无可刷写转发固件。板载 bootloader、引脚与低频时钟在硬件适配阶段核对。ANT 官方要求低频时钟误差不超过 ±50 ppm。

BLE 接收端先按标准 Heart Rate Service（0x180D）/Heart Rate Measurement（0x2A37）实现。最终接收方式依据设备实际输出确认；心率广播的产品名称不一定代表心率值直接位于广告包内。
