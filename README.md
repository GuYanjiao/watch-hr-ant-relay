# Watch HR ANT Relay

把手表的蓝牙心率转发为 ANT+ 心率，让支持 ANT+ 的码表接收实时心率。

```text
手表心率共享 ── BLE / GATT ──► nRF52840 ── ANT+ ──► 码表
```

已在 **Pro Micro nRF52840 红板**上完成实机联调：开发板持续接收手表心率，码表成功搜索到设备并显示实时心率。ANT+ 设备号为 **4242**。

## 功能

- 自动扫描标准心率服务 `0x180D`，连接第一个匹配设备并订阅心率特征 `0x2A37`。
- 解析 8/16 位心率、接触状态、能量字段和 RR 间隔；心率值按 ANT 的 8 位范围转发。
- 以约 4 Hz 发送 ANT+ HRM 数据及背景页。
- 断线后重新扫描；5 秒无新心率、接触丢失或零心率时停止 ANT 广播；15 秒没有合法通知时重新连接。
- 通过 USB 串口查看运行状态、进入 UF2 刷写模式。

这里的“心率共享”需要提供**可连接的标准 BLE 心率服务**，不是解析厂商私有广告包。首次使用时只开启目标设备的心率共享，并断开手机等设备对该服务的占用。

没有 RR 间隔时，固件根据 BPM 推算心跳次数和时间，**不能作为真实 HRV 数据使用**。本次实测手表提供 BPM，没有 RR 间隔。

## 硬件

当前适配和验证的是 Pro Micro 外形的 nRF52840 红板，启动程序信息为：

| 项目 | 已验证配置 |
| --- | --- |
| 板型标识 | `nRF52840-nicenano` |
| 启动程序 | UF2 0.6.0 |
| 原厂 SoftDevice | S140 6.1.1 |
| 启动磁盘卷标 | `NICENANO` |
| 应用起始地址 | `0x26000` |
| 应用区域终点 | `0xEC000` |

固件使用高频晶振合成低频时钟（SYNTH），并显式启动高频晶振。这是本次红板实测所需的时钟处理，功耗尚未优化。其他启动程序版本或克隆板需要核对分区及硬件配置。

## 目录

```text
firmware/
  src/main.c                 BLE 连接、ANT 转发及 USB 控制
  src/hr_measurement.[ch]     BLE 心率数据解析
  boards/                    红板配置、静态分区表
scripts/
  build.ps1                  构建红板 UF2
  flash.ps1                  检查固件与目标磁盘并刷写
  bootloader.py              让运行中的固件进入 UF2 模式
  monitor.py                 读取 USB 日志
  verify_uf2.py               检查 UF2 数据块及启动入口
tests/                      ARM 心率解析测试
.vscode/                     VS Code 扩展和默认构建任务
requirements-dev.txt         Python 工具及测试依赖
```

## 开发环境

当前构建脚本面向 **Windows / PowerShell**，编辑器使用 VS Code。

| 依赖 | 版本 / 路径 |
| --- | --- |
| nRF Connect SDK | `v3.2.4`，`C:\ncs\v3.2.4` |
| ANT Add-on | `v2.1.1`，`C:\ncs\addons\sdk-ant-v2.1.1` |
| Nordic 工具链 | `fd21892d0f`，`C:\ncs\toolchains\fd21892d0f` |
| nrfutil | `C:\ncs\bin\nrfutil.exe`，安装 SDK Manager |
| Python | 3.10 或以上 |
| VS Code 扩展 | nRF Connect Extension Pack、C/C++ |

先按 [Nordic SDK 安装说明](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation.html)安装工具。仓库使用固定 SDK/工具链版本；如果改动安装位置，需要相应修改构建、刷写和测试脚本。

SDK 和 ANT Add-on 的安装命令：

```powershell
nrfutil install sdk-manager
nrfutil sdk-manager install v3.2.4 --install-dir C:\ncs --region cn
git clone --branch v2.1.1 --depth 1 https://github.com/ant-nrfconnect/sdk-ant.git C:\ncs\addons\sdk-ant-v2.1.1
```

ANT Add-on 与 SDK 的版本对应关系见[官方兼容表](https://ant-nrfconnect.github.io/doc/compatibility.html)。

在仓库根目录安装 Python 依赖：

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-dev.txt
```

## 构建

在 VS Code 打开仓库文件夹，按 `Ctrl+Shift+B`，或执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1
```

脚本会把应用源码复制到 `C:\ncs\apps\watch-hr-ant-relay` 后构建，以避开中文路径对工具链的影响。UF2 产物：

```text
C:\ncs\build\watch-hr-promicro\watch-hr-ant-relay\zephyr\zephyr.uf2
```

静态分区表同时约束链接地址和写入地址；构建结束后自动验证 UF2 芯片系列、每个数据块的范围、初始栈及复位入口。

## 刷写与使用

1. 用 USB 数据线连接开发板。首次刷写时，在半秒内将 **RST 与 GND 短接两次，每次立即松开**，使 `NICENANO` 磁盘出现。RAW 是供电引脚，不用于复位。
2. 运行刷写命令：

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\flash.ps1
   ```

3. 开启手表的心率共享，将手表放在开发板附近。
4. 在码表中添加 ANT+ 心率传感器，选择设备号 **4242**。

刷写工具只接受匹配板型的 `NICENANO` 磁盘，并检查应用 UF2 地址；保留原厂启动程序和 SoftDevice 区域，不执行整片擦除。

已运行本项目固件时，可以通过电脑进入 UF2 模式：

```powershell
.\.venv\Scripts\python.exe scripts\bootloader.py
```

然后再次执行 `scripts\flash.ps1`。USB 仅用于供电、日志和刷写；正常转发无需手机中转。

## 日志与测试

读取运行日志：

```powershell
.\.venv\Scripts\python.exe scripts\monitor.py --seconds 30
```

工具自动识别固件 USB 串口，将日志保存为 `artifacts/relay-serial.log`。正常转发时可看到 `BLE=connected`、`fresh_HR=1`、`ANT=1` 和实时 BPM。

解析测试：

```powershell
.\.venv\Scripts\python.exe tests\test_parser.py
```

测试交叉编译实际的 C 解析器，并通过 Unicorn 执行 ARM 指令，覆盖 8/16 位心率、能量字段、RR 间隔、接触状态、数值范围及截断/格式错误输入。

已验证编译、UF2 刷写、USB 进入启动模式、实际 BLE 心率接收和码表 ANT+ 接收。断线恢复、长时间稳定性及电池功耗尚未单独完成实机测试。

## 依赖与许可

仓库不包含 Nordic SDK、ANT 协议栈、网络密钥、原厂固件备份或实机日志。ANT 使用 SDK 的个人非商业评估配置，其他用途应按 [ANT 官方许可](https://developer.garmin.com/ant-program/)取得相应授权。

第三方源码归属及许可见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
