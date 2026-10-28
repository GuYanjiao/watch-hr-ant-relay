# Watch HR ANT Relay

目标：nRF52840 接收手表的 BLE 心率数据，按 ANT+ 心率设备规范发送。

开发板：Pro Micro 外形 nRF52840 红板。编辑器：VS Code。

## 开发环境

- 源码在当前目录，Git 本地仓库，沿用用户现有提交身份。
- Nordic 工具安装在 C:\ncs，避免把 SDK 和编译器放入 Git。
- 版本基线：nRF Connect SDK v3.2.4，ANT Add-on v2.1.1。
- ANT 与 SDK 版本依照官方兼容表固定：https://ant-nrfconnect.github.io/doc/compatibility.html
- 官方 ANT 安装指南：https://ant-nrfconnect.github.io/doc/getting_started.html

环境已配置，第一版转发固件已完成参考板构建。板载 bootloader、引脚与低频时钟在硬件适配阶段核对。ANT 官方要求低频时钟误差不超过 ±50 ppm。

BLE 接收端先按标准 Heart Rate Service（0x180D）/Heart Rate Measurement（0x2A37）实现。最终接收方式依据设备实际输出确认；心率广播的产品名称不一定代表心率值直接位于广告包内。

## 已验证的环境

2026-10-28：Git 2.56.0、nrfutil 8.2.1、SDK Manager 1.16.1、Device 2.23.0；SDK v3.2.4 与工具链 fd21892d0f 已安装。ARM GCC 12.2.0、Python 3.12.4、west 1.4.0。VS Code 已安装 Nordic 扩展包与 C/C++ 扩展。

ANT 源码：C:\ncs\addons\sdk-ant-v2.1.1，标签 v2.1.1，提交 08cecef477745fe2c34a5ef58d7b1dafa1f3a444。

以下两个参考板构建均完整编译、链接并生成 merged.hex：

- scripts/build-smoke.ps1：hello_world，产物 C:\ncs\build\watch-hr-smoke\merged.hex。
- scripts/build-ant-smoke.ps1：ANT+ hrm_tx，产物 C:\ncs\build\watch-hr-ant-smoke\merged.hex，启用个人评估配置。

两者板型都是 nrf52840dk/nrf52840，仅验证环境，不代表 Pro Micro 红板的引脚、bootloader 或无线功能已验证。未刷写任何设备。

VS Code 菜单“终端 → 运行任务”可执行环境检查与编译。打开 watch-hr-ant-relay.code-workspace 即进入工作区。Git 使用原有用户身份，未配置远程仓库，也未推送。初始化提交因已有 GPG 签名等待无响应而使用单次不签名提交，全局签名配置保持不变。

## 第一版转换固件

源码：`firmware/`。在 VS Code 中按 Ctrl+Shift+B，或运行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-relay.ps1
```

脚本将源码同步到专用目录 `C:\ncs\apps\watch-hr-ant-relay` 再编译，以避开中文路径问题。构建产物在 `C:\ncs\build\watch-hr-relay\merged.hex`。

当前实现：

- 主动扫描标准 BLE 心率服务 0x180D，连接第一个匹配设备，订阅 0x2A37。
- 解析 8/16 位心率、传感器接触状态、能量字段与 RR 间隔；超过 ANT 8 位范围的 BPM 不转发。
- 使用 ANT HRM profile 以 4 Hz 发送心率及背景页，开发设备号为 4242。
- 收到有效心率才开启 ANT；断线、零心率、接触丢失或 5 秒无新心率时停止广播。
- BLE 断开自动重扫，15 秒没有合法通知时主动重连；订阅被拒绝也重连。
- 有 RR 间隔时累计真实逐搏数据；没有 RR 时根据 BPM 推算心跳次数/时间，不能作为真实 HRV 数据使用。

本版支持“可连接的标准 BLE 心率服务”。不解析厂家私有的广告包心率，也不提供设备选择界面。首次联调时只开启目标手表的心率共享功能。

### 验证与当前边界

参考板配置 `nrf52840dk/nrf52840` 已完整编译、链接，BLE 和 ANT 同时启用。未在 Pro Micro 红板上刷写或验证无线链路。当前 `merged.hex` 是参考板构建，不应直接刷入红板；仍需确认它的 bootloader、分区与低频晶振后生成匹配版本。

解析测试直接模拟运行交叉编译后的 ARM C 代码：

```powershell
C:\Python315\python.exe -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r tests\requirements.txt
.\.venv\Scripts\python.exe tests\test_parser.py
```

已通过：8/16 位心率、RR 与能量字段、接触状态、零心率、范围限制、截断及格式错误数据包。该测试不代替实机 BLE/ANT 射频联调。

ANT 依赖使用 SDK 自带的个人评估配置；依赖许可保留在 `C:\ncs\addons\sdk-ant-v2.1.1\LICENSE.txt`，profile 的 Shared Source License 见该 SDK 对应源码文件头。
