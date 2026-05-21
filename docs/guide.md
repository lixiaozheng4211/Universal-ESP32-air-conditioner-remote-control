# Universal ESP32 空调串口遥控器 — 小白完全指南

## 1. 项目简介

### 这是什么？

这是一个**万能空调红外遥控器**项目。简单来说：

- 你有一块 ESP32-S3 开发板，上面接了一个红外发射管
- 电脑（或手机）通过 USB 线连接这块开发板
- 电脑发送文字命令（比如"开机、制冷、26度"），开发板就会发射对应的红外信号
- 空调收到红外信号后就会执行对应操作

**它能做什么？**

- 支持 40+ 个空调品牌（美的、格力、海尔、大金、松下等）
- 支持开关机、调温、切模式、调风速、控制摆风
- 可以同时管理多台不同品牌的空调
- 提供 Windows 桌面软件（Qt 上位机）方便操作
- 也可以接 Android 手机控制

### 谁适合用这个项目？

- 想用电脑/手机统一控制多台空调的人
- 想做智能家居红外控制的开发者
- 想学习 ESP32 + 红外通信的学生

---

## 2. 系统架构

### 整体工作流程

```
┌─────────────┐    USB串口     ┌─────────────┐    红外信号    ┌─────────┐
│  电脑/手机   │ ──────────── → │  ESP32-S3   │ ──────────── → │  空调    │
│  (上位机)    │ ← ──────────── │  (遥控器)    │               │         │
└─────────────┘   文字命令/响应  └─────────────┘               └─────────┘
```

### 详细架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                        上位机 (电脑/手机)                             │
├─────────────────────────────────────────────────────────────────────┤
│  Qt 桌面软件 (Windows)          │  Android App                      │
│  - 串口连接管理                  │  - USB Host 串口                  │
│  - 空调添加向导                  │  - 同样的文字命令                  │
│  - 卡片式空调管理                │                                   │
│  - 批量控制                     │                                   │
└────────────────┬────────────────┴───────────────────────────────────┘
                 │ USB 串口 (115200 8N1)
                 │ 文本协议: PING / CATALOG / AC / IRTEST
                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     ESP32-S3 固件                                     │
├─────────────────────────────────────────────────────────────────────┤
│  串口协议解析器         →  空调命令验证器      →  红外发射驱动        │
│  (serial_protocol)        (air_conditioner)      (irac_backend)     │
│                                                                      │
│  品牌/遥控器目录 (catalog)                                           │
│  - 40+ 品牌定义                                                      │
│  - 每个品牌多种遥控器型号                                             │
│  - 温度范围、风速、摆风能力声明                                       │
├─────────────────────────────────────────────────────────────────────┤
│  IRremoteESP8266 库 (60+ 红外协议实现)                               │
└────────────────┬────────────────────────────────────────────────────┘
                 │ GPIO4 → 红外 LED
                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        空调 (接收红外信号)                            │
└─────────────────────────────────────────────────────────────────────┘
```

### 通信方式说明

电脑和 ESP32 之间用的是**文本命令**，不是二进制协议。就像你在聊天软件里打字一样，发一行文字，收一行回复：

```
电脑发送:  PING
ESP32回复: OK PONG

电脑发送:  AC remote=midea_standard power=1 mode=cool temp=26 fan=auto swingv=off swingh=off
ESP32回复: OK SENT remote=midea_standard action=state power=1 mode=cool temp=26.0
```

这意味着你甚至可以用任何串口调试工具（比如 PuTTY、串口助手）手动输入命令来控制空调，不一定非要用 Qt 软件。

---

## 3. 硬件准备清单

### 必需硬件

| 序号 | 物品 | 说明 |
| --- | --- | --- |
| 1 | ESP32-S3 开发板 | 任意带 USB 接口的 ESP32-S3 开发板均可 |
| 2 | 红外发射管 | 940nm 红外 LED，建议用大功率型号（发射距离更远） |
| 3 | NPN 三极管 | 如 S8050，用于驱动红外 LED（ESP32 GPIO 直驱电流不够） |
| 4 | 电阻 | 基极限流电阻 1kΩ + LED 限流电阻（根据 LED 参数选择） |
| 5 | USB 数据线 | 必须是数据线，不能是只充电的线 |
| 6 | 面包板 + 杜邦线 | 用于搭建电路（或者你直接焊接也行） |

### 可选硬件（调试用）

| 物品 | 用途 |
| --- | --- |
| 红外接收模块 | 验证红外信号是否正常发射（如 VS1838B） |
| 示波器 | 观察红外载波波形 |
| 逻辑分析仪 | 分析红外编码时序 |

### 接线方式

```
ESP32-S3 GPIO4 ──→ 1kΩ电阻 ──→ NPN三极管(基极)
                                    │
                              (集电极) ──→ 红外LED(阴极) ──→ LED限流电阻 ──→ 3.3V/5V
                                    │
                              (发射极) ──→ GND
```

> 红外发射引脚默认是 GPIO4，如需修改，编辑文件：
> `airconditioner_control_main/main/core/ac_config.h`

---

## 4. 开发环境搭建

本项目有两个部分需要编译：ESP32 固件 和 Qt 上位机。如果你只想用上位机控制空调（固件已经烧好），可以跳过 4.1 直接看 4.2。

### 4.1 ESP-IDF 环境（编译固件用）

ESP-IDF 是乐鑫官方的 ESP32 开发框架。本项目使用 **v5.5.3** 版本。

#### 第一步：下载安装 ESP-IDF

1. 访问乐鑫官网下载 ESP-IDF 离线安装包：
   - Windows 用户推荐使用官方安装器：[ESP-IDF Tools Installer](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.3/esp32s3/get-started/windows-setup.html)
   - 安装时选择 ESP-IDF v5.5.3
   - 安装路径建议：`C:\esp\v5.5.3\esp-idf`

2. 安装完成后，你会得到一个 "ESP-IDF PowerShell" 快捷方式，打开它就是配置好环境的终端。

#### 第二步：验证安装

打开 ESP-IDF PowerShell 终端，输入：

```powershell
idf.py --version
```

如果显示版本号（如 `ESP-IDF v5.5.3`），说明安装成功。

#### 手动激活环境（如果不用快捷方式）

如果你想在普通 PowerShell 中使用 ESP-IDF：

```powershell
. C:\esp\v5.5.3\esp-idf\export.ps1
```

这条命令会把 ESP-IDF 工具链加入当前终端的 PATH。每次打开新终端都需要执行一次。

### 4.2 Qt6 环境（编译上位机用）

> 如果你只想直接运行已编译好的上位机，跳过这一节，直接看第 6 节的"直接运行"部分。

本项目的 Qt 上位机使用 Qt6 + CMake + Ninja 构建，Windows 下推荐使用 MSYS2 的 UCRT64 环境。

#### 第一步：安装 MSYS2

1. 下载 MSYS2 安装包：[https://www.msys2.org/](https://www.msys2.org/)
2. 安装到默认路径 `C:\msys64`
3. 安装完成后打开 "MSYS2 UCRT64" 终端

#### 第二步：安装 Qt6 和构建工具

在 MSYS2 UCRT64 终端中执行：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base
pacman -S mingw-w64-ucrt-x86_64-qt6-serialport
pacman -S mingw-w64-ucrt-x86_64-cmake
pacman -S mingw-w64-ucrt-x86_64-ninja
pacman -S mingw-w64-ucrt-x86_64-gcc
```

#### 第三步：验证安装

```bash
qmake6 --version
cmake --version
ninja --version
```

都能正常输出版本号即可。

#### 关键提示

在 Windows PowerShell 中编译和运行 Qt 程序前，必须先把 MSYS2 的 bin 目录加到 PATH 最前面：

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
```

否则系统可能找不到 Qt 的 DLL 文件，导致编译失败或程序无法启动。

---

## 5. 固件编译与烧录

### 5.1 编译固件

1. 打开 ESP-IDF PowerShell 终端（或手动执行 `export.ps1`）
2. 进入项目根目录：

```powershell
cd C:\Users\你的用户名\Desktop\airconditioner_software
```

3. 编译固件：

```powershell
idf.py -C airconditioner_control_main build
```

首次编译需要几分钟（下载依赖 + 编译所有源码），后续修改代码后重新编译会快很多。

编译成功后会显示类似：

```
Project build complete. To flash, run:
 idf.py flash
```

### 5.2 烧录到 ESP32

1. 用 USB 数据线连接 ESP32-S3 到电脑
2. 打开设备管理器，找到 ESP32 对应的 COM 口（比如 COM3）
3. 执行烧录命令：

```powershell
idf.py -C airconditioner_control_main -p COM3 flash
```

> 把 `COM3` 换成你实际的端口号。

烧录成功后会显示 `Hard resetting via RTS pin...`。

### 5.3 查看串口输出（调试）

烧录完成后，可以打开串口监视器查看 ESP32 的运行日志：

```powershell
idf.py -C airconditioner_control_main -p COM3 monitor
```

按 `Ctrl+]` 退出监视器。

也可以一步完成编译+烧录+监视：

```powershell
idf.py -C airconditioner_control_main -p COM3 flash monitor
```

### 5.4 常见编译问题

| 问题 | 解决方法 |
| --- | --- |
| `idf.py` 命令找不到 | 确认已执行 `export.ps1` 或使用 ESP-IDF 专用终端 |
| 找不到 COM 口 | 检查 USB 线是否是数据线，检查驱动是否安装 |
| 编译报错缺少组件 | 执行 `idf.py -C airconditioner_control_main reconfigure` |
| 烧录失败 | 确认 COM 口没被其他程序占用，尝试按住 ESP32 的 BOOT 键再烧录 |

---

## 6. Qt 上位机

### 6.1 直接运行（已编译好的版本）

如果你有已编译好的发布包（`release/` 目录下），直接使用即可：

1. 解压发布包（如果是 zip）
2. 找到 `UniversalAcHost` 文件夹
3. 双击 `UniversalAcHost.bat` 启动

> 注意：不要单独移动 exe 文件，必须保持文件夹结构完整。

### 6.2 从源码编译

如果你需要修改上位机代码或没有现成的发布包：

1. 打开 PowerShell，先设置环境：

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
```

2. 配置构建：

```powershell
qt-cmake.bat -S qt_host -B qt_host/build -G Ninja
```

3. 编译：

```powershell
cmake --build qt_host/build
```

4. 运行：

```powershell
.\qt_host\build\universal_ac_host.exe
```

### 6.3 使用上位机

#### 连接 ESP32

1. 用 USB 线连接 ESP32 到电脑
2. 打开上位机软件
3. 在左上角下拉框选择对应的 COM 口
4. 点击"连接"
5. 连接成功后，软件会自动发送 `PING` 检测设备，并读取 `CATALOG` 获取支持的品牌列表

#### 添加空调

1. 点击"添加空调"按钮
2. 选择你的空调品牌（如"美的"）
3. 软件会列出该品牌下所有候选遥控器
4. 对准空调，逐个测试候选遥控器（软件会发送开机命令）
5. 如果空调有反应（听到"嘀"一声或看到显示屏亮起），说明这个遥控器匹配
6. 点击确认保存

#### 控制空调

- **单击**卡片：选中该空调
- **双击**卡片：打开详细控制面板（温度、模式、风速、摆风）
- **勾选**多个卡片：进行批量操作
- **开启空调**：开启已勾选的空调（没勾选则开启全部）
- **关闭所有空调**：关闭已勾选的空调（没勾选则关闭全部）
- **批量设置**：统一设置已勾选空调的参数
- **红外测试**：检查红外硬件是否正常工作

#### 串口日志

点击"串口日志"按钮可以查看通信记录：
- `TX`：软件发送给 ESP32 的命令
- `RX`：ESP32 返回的响应

遇到空调没反应时，先看日志确认命令是否发送成功。

---

## 7. 通信协议说明

这一节介绍电脑/手机和 ESP32 之间的通信规则。如果你只用 Qt 上位机，不需要了解这些细节；如果你想自己写程序控制空调（比如写个 Python 脚本），这一节很重要。

### 7.1 串口参数

| 参数 | 值 |
| --- | --- |
| 波特率 | 115200 |
| 数据位 | 8 |
| 校验位 | 无 (None) |
| 停止位 | 1 |
| 流控 | 无 |
| 行结束符 | `\n`（换行符） |
| 编码 | UTF-8 或 ASCII |

### 7.2 命令列表

#### PING — 检测设备是否在线

```
发送: PING
回复: OK PONG
```

#### HELP — 查看支持的命令

```
发送: HELP
回复: OK COMMANDS PING CATALOG AC(action=state/power/temp/mode/fan/swingv/swingh) IRTEST
```

#### CATALOG — 获取支持的品牌和遥控器

```
发送: CATALOG
回复:
OK CATALOG remotes=88
CAT BRAND id=midea name="Midea" first_child=16 next_sibling=1
CAT REMOTE id=midea_standard brand=midea name="Midea standard" temp=17-30 fan=1 swingv=1 swingh=0 driver="IRac" next_sibling=17
...
OK CATALOG END
```

#### AC — 控制空调

```
发送: AC remote=<遥控器ID> action=<动作> power=<0或1> mode=<模式> temp=<温度> fan=<风速> swingv=<上下风> swingh=<左右风>
回复: OK SENT remote=<id> action=<action> power=<值> mode=<值> temp=<值>
```

参数说明：

| 参数 | 必填 | 可选值 | 说明 |
| --- | --- | --- | --- |
| `remote` | 是 | 来自 CATALOG | 遥控器 ID |
| `action` | 否 | state/power/temp/mode/fan/swingv/swingh | 默认 state（完整同步） |
| `power` | 是 | 0 / 1 | 0=关机, 1=开机 |
| `mode` | 是 | auto/cool/heat/dry/fan | 模式 |
| `temp` | 是 | 17-30（视遥控器而定） | 温度 |
| `fan` | 是 | auto/low/med/high/max | 风速 |
| `swingv` | 是 | off/auto | 上下摆风 |
| `swingh` | 是 | off/auto | 左右摆风 |

#### IRTEST — 红外硬件测试

```
发送: IRTEST freq=38000 mode=nec count=4 duty=33
回复: OK IRTEST freq=38000 mode=nec count=4 duty=33
```

### 7.3 错误响应

所有错误以 `ERR` 开头：

```
ERR UNKNOWN_REMOTE midea_xxx        ← 遥控器ID不存在
ERR BAD_ACTION use state/power/...  ← action参数错误
ERR BAD_TEMP temperature out of...  ← 温度超出范围
ERR SEND_FAILED midea_rn02s13      ← 红外发送失败
```

### 7.4 用 Python 控制空调的示例

```python
import serial
import time

# 打开串口
ser = serial.Serial('COM3', 115200, timeout=2)
time.sleep(1)

# 检测设备
ser.write(b'PING\n')
response = ser.readline().decode().strip()
print(f"设备响应: {response}")  # 应该是 OK PONG

# 开机 - 美的空调，制冷26度
ser.write(b'AC remote=midea_standard power=1 mode=cool temp=26 fan=auto swingv=off swingh=off\n')
response = ser.readline().decode().strip()
print(f"控制结果: {response}")

# 关机
ser.write(b'AC remote=midea_standard action=power power=0 mode=cool temp=26 fan=auto swingv=off swingh=off\n')
response = ser.readline().decode().strip()
print(f"关机结果: {response}")

ser.close()
```

---

## 8. 支持的空调品牌

当前固件支持以下品牌（共 40+ 个），每个品牌可能有多种遥控器型号：

### 国产品牌

| 品牌 | 英文ID前缀 | 说明 |
| --- | --- | --- |
| 美的 | midea | 多种型号，含 RN02S13 等 |
| 格力 | gree | 多种型号 |
| 海尔 | haier | 多种型号 |
| TCL | tcl | |
| 科龙 | kelon | |
| 奥克斯 | aux | |

### 日系品牌

| 品牌 | 英文ID前缀 |
| --- | --- |
| 大金 (Daikin) | daikin |
| 日立 (Hitachi) | hitachi |
| 松下 (Panasonic) | panasonic |
| 三菱电机 | mitsubishi |
| 三菱重工 | mitsubishi_heavy |
| 东芝 (Toshiba) | toshiba |
| 夏普 (Sharp) | sharp |
| 三洋 (Sanyo) | sanyo |
| 富士通 (Fujitsu) | fujitsu |
| Corona | corona |

### 韩系品牌

| 品牌 | 英文ID前缀 |
| --- | --- |
| 三星 (Samsung) | samsung |
| LG | lg |

### 欧美及其他品牌

| 品牌 | 英文ID前缀 |
| --- | --- |
| 开利 (Carrier) | carrier |
| 惠而浦 (Whirlpool) | whirlpool |
| 博世 (Bosch) | bosch |
| 德龙 (Delonghi) | delonghi |
| Kelvinator | kelvinator |
| Argo | argo |
| Vestel | vestel |
| Trotec | trotec |
| Coolix | coolix |
| Airton | airton |
| Airwell | airwell |
| Amcor | amcor |
| Ecoclim | ecoclim |
| Eurom | eurom |
| Goodweather | goodweather |
| Mirage | mirage |
| Neoclima | neoclima |
| Rhoss | rhoss |
| Teknopoint | teknopoint |
| Technibel | technibel |
| TECO | teco |
| Truma | truma |
| Voltas | voltas |
| Transcold | transcold |

> 具体支持哪些遥控器型号，连接 ESP32 后发送 `CATALOG` 命令即可查看完整列表。

---

## 9. 常见问题排查

### 硬件相关

#### Q: 空调完全没反应

按以下顺序排查：

1. **确认串口连接正常**：Qt 上位机能连接并收到 `OK PONG`
2. **测试红外发射**：点击"红外测试" → 38K 测试，用手机摄像头对准红外 LED 看是否有紫色闪光（肉眼看不到红外光，但手机摄像头可以）
3. **检查接线**：GPIO4 → 三极管基极 → 红外 LED，确认方向没接反
4. **确认品牌和遥控器型号**：同一品牌可能有多种遥控器协议，逐个尝试
5. **距离和角度**：红外 LED 需要对准空调的接收窗口，距离不要太远

#### Q: 红外测试有信号但空调不响应

- 可能选错了遥控器型号，换一个候选试试
- 确认红外 LED 的载波频率匹配（大多数空调用 38kHz）
- 检查红外 LED 发射功率是否足够（距离太远信号衰减）

#### Q: USB 线连上但找不到 COM 口

- 确认是**数据线**不是充电线（充电线只有电源线没有数据线）
- 安装 ESP32-S3 的 USB 驱动（通常是 CP2102 或 CH340）
- 尝试换一个 USB 口
- 打开设备管理器查看是否有未识别设备

### 软件相关

#### Q: Qt 上位机打不开

- 确认是双击 `UniversalAcHost.bat`，不是直接运行 exe
- 确认整个文件夹都已解压，不能只复制单个文件
- 如果从源码编译运行，确认 `$env:PATH` 包含 `C:\msys64\ucrt64\bin`

#### Q: 编译固件报错

- 确认 ESP-IDF 版本是 v5.5.3
- 确认已执行 `export.ps1` 激活环境
- 尝试清理后重新编译：`idf.py -C airconditioner_control_main fullclean` 然后重新 `build`

#### Q: Qt 编译报错找不到 SerialPort

在 MSYS2 UCRT64 终端中安装：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-serialport
```

#### Q: USB 断开后重连失败

- 重新插拔 USB 线
- 在上位机中重新选择 COM 口
- 如果 COM 口号变了（比如从 COM3 变成 COM4），需要选择新的端口

---

## 10. 代码结构说明

如果你想了解或修改代码，这一节帮你快速定位。

### 10.1 固件代码结构

```
airconditioner_control_main/
├── main/                          ← 固件源代码
│   ├── main.cpp                   ← 程序入口，初始化串口和红外
│   ├── core/                      ← 核心逻辑
│   │   ├── ac_config.h            ← 硬件配置（GPIO引脚、波特率）
│   │   ├── ac_types.h             ← 数据类型定义（模式、风速等枚举）
│   │   └── air_conditioner.cpp/h  ← 空调命令验证（检查温度范围等）
│   ├── protocol/                  ← 串口协议
│   │   └── serial_protocol.cpp/h  ← 解析文本命令，分发到对应处理函数
│   ├── drivers/                   ← 红外驱动
│   │   ├── irac_backend.cpp/h     ← 调用 IRremoteESP8266 发射红外
│   │   └── ir_test.cpp/h          ← 红外硬件测试功能
│   ├── catalog/                   ← 品牌目录
│   │   └── ac_catalog.cpp/h       ← 管理所有品牌和遥控器的注册表
│   └── remotes/                   ← 各品牌遥控器定义
│       ├── midea_remotes.cpp      ← 美的
│       ├── gree_remotes.cpp       ← 格力
│       ├── haier_remotes.cpp      ← 海尔
│       └── ...                    ← 其他品牌
├── components/                    ← 第三方库
│   ├── IRremoteESP8266/           ← 红外协议库（核心依赖）
│   └── arduino/                   ← Arduino 兼容层
├── CMakeLists.txt                 ← 构建配置
└── sdkconfig                      ← ESP-IDF 芯片配置
```

**如果你想...**

- 修改红外引脚 → 编辑 `main/core/ac_config.h`
- 添加新品牌 → 在 `main/remotes/` 下新建文件，在 `main/catalog/ac_catalog.cpp` 中注册
- 修改串口协议 → 编辑 `main/protocol/serial_protocol.cpp`
- 调整红外发射逻辑 → 编辑 `main/drivers/irac_backend.cpp`

### 10.2 Qt 上位机代码结构

```
qt_host/
├── src/
│   ├── main.cpp                   ← 程序入口
│   ├── domain/                    ← 数据模型
│   │   ├── ac_catalog.cpp/h       ← 解析固件返回的品牌目录
│   │   └── ac_store.cpp/h         ← 本地空调库（保存到 JSON）
│   ├── serial/                    ← 串口通信
│   │   └── serial_controller.cpp/h ← 串口连接、发送、接收
│   ├── ui/                        ← 界面
│   │   ├── main_window.cpp/h      ← 主窗口（卡片网格）
│   │   ├── ac_control_dialog.cpp/h ← 详细控制面板
│   │   └── ...                    ← 其他 UI 组件
│   └── workflow/                   ← 业务流程
│       ├── ac_discovery_wizard.cpp/h ← 添加空调向导
│       └── known_ac_runner.cpp/h    ← 批量控制逻辑
└── CMakeLists.txt                 ← 构建配置
```

**如果你想...**

- 修改界面布局 → 编辑 `src/ui/` 下的文件
- 修改串口通信逻辑 → 编辑 `src/serial/serial_controller.cpp`
- 修改空调添加流程 → 编辑 `src/workflow/ac_discovery_wizard.cpp`
- 修改本地存储格式 → 编辑 `src/domain/ac_store.cpp`

---

## 附录：快速参考卡片

### 固件编译烧录（三步走）

```powershell
# 1. 激活环境
. C:\esp\v5.5.3\esp-idf\export.ps1

# 2. 编译
idf.py -C airconditioner_control_main build

# 3. 烧录
idf.py -C airconditioner_control_main -p COM3 flash
```

### Qt 上位机编译（三步走）

```powershell
# 1. 设置 PATH
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH

# 2. 配置
qt-cmake.bat -S qt_host -B qt_host/build -G Ninja

# 3. 编译
cmake --build qt_host/build
```

### 常用串口命令

```
PING                                    ← 检测设备
CATALOG                                 ← 查看支持的品牌
AC remote=midea_standard power=1 mode=cool temp=26 fan=auto swingv=off swingh=off  ← 开机制冷26度
AC remote=midea_standard action=power power=0 mode=cool temp=26 fan=auto swingv=off swingh=off  ← 关机
IRTEST freq=38000 mode=nec count=4 duty=33  ← 红外测试
```

