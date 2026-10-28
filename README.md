# Watch HR ANT Relay

目标：nRF52840 接收手表的 BLE 心率数据，按 ANT+ 心率设备规范发送。

开发板：Pro Micro 外形 nRF52840 红板。编辑器：VS Code。

## 开发环境

- 源码在当前目录，Git 本地仓库，沿用用户现有提交身份。
- Nordic 工具安装在 C:\ncs，避免把 SDK 和编译器放入 Git。
- 版本基线：nRF Connect SDK v3.2.4，ANT Add-on v2.1.1。
- ANT 与 SDK 版本依照官方兼容表固定：https://ant-nrfconnect.github.io/doc/compatibility.html
- 官方 ANT 安装指南：https://ant-nrfconnect.github.io/doc/getting_started.html

环境已配置，尚无可刷写转发固件。板载 bootloader、引脚与低频时钟在硬件适配阶段核对。ANT 官方要求低频时钟误差不超过 ±50 ppm。

BLE 接收端先按标准 Heart Rate Service（0x180D）/Heart Rate Measurement（0x2A37）实现。最终接收方式依据设备实际输出确认；心率广播的产品名称不一定代表心率值直接位于广告包内。

## 已验证的环境

2026-10-28：Git 2.56.0、nrfutil 8.2.1、SDK Manager 1.16.1、Device 2.23.0；SDK v3.2.4 与工具链 fd21892d0f 已安装。ARM GCC 12.2.0、Python 3.12.4、west 1.4.0。VS Code 已安装 Nordic 扩展包与 C/C++ 扩展。

ANT 源码：C:\ncs\addons\sdk-ant-v2.1.1，标签 v2.1.1，提交 08cecef477745fe2c34a5ef58d7b1dafa1f3a444。

以下两个参考板构建均完整编译、链接并生成 merged.hex：

- scripts/build-smoke.ps1：hello_world，产物 C:\ncs\build\watch-hr-smoke\merged.hex。
- scripts/build-ant-smoke.ps1：ANT+ hrm_tx，产物 C:\ncs\build\watch-hr-ant-smoke\merged.hex，启用个人评估配置。

两者板型都是 nrf52840dk/nrf52840，仅验证环境，不代表 Pro Micro 红板的引脚、bootloader 或无线功能已验证。未刷写任何设备。

VS Code 菜单“终端 → 运行任务”可执行环境检查与编译。打开 watch-hr-ant-relay.code-workspace 即进入工作区。Git 使用原有用户身份，未配置远程仓库，也未推送。初始化提交因已有 GPG 签名等待无响应而使用单次不签名提交，全局签名配置保持不变。
