# Universal ESP32 空调串口遥控器

这个仓库实现了一个“串口空调遥控器”：上位设备通过 USB 串口给 ESP32-S3 发送文本命令，ESP32-S3 再选择对应空调遥控器协议并发射红外。

当前仓库包含两部分：

- `airconditioner_control_main/`：ESP32 固件主工程，负责串口协议解析、遥控器目录、IRremoteESP8266 红外发送。
- `qt_host/`：Qt6 Widgets 测试/参考上位机，用来在电脑上验证串口协议、添加空调和测试红外硬件。
- `android_app/`：Android 原生上位机，用手机通过 USB OTG + CH34x 串口控制 ESP32。

Qt 上位机只是调试工具，不是最终平台限制。Android、Windows、Linux 或其它主控只要能打开串口并发送同样的文本命令，就可以控制 ESP32 发射空调红外。

Android 端开发请优先阅读：

```text
docs/android_serial_protocol.md
```

## Android 手机上位机

Android 工程位于：

```text
android_app/
```

已构建好的调试 APK 放在：

```text
release/Android_apk/app-debug.apk
```

Android 端使用 Android USB Host 直接访问 CH340K/CH34x USB-UART，不需要在手机系统里安装 CH340 驱动。手机需要支持 OTG；如果 USB-C 直连无法枚举设备，建议使用 OTG 转接板或带外部供电的 OTG Hub。当前 App 已额外加入 `1A86:7522` 到 CH34x 驱动匹配表，常见 `1A86:7523` 也由串口库默认支持。

Android App 功能按 Qt 上位机模式实现：

- USB 连接、断开、USB 诊断、`PING`/`CATALOG` 自动握手。
- 可选空调目录弹窗，按品牌查看固件返回的候选遥控器。
- 添加空调向导：选择品牌，逐个候选测试开机和调温，确认后保存。
- 已保存空调列表支持搜索、排序、当前选择、多选、全选、清空选择。
- 单击空调设置当前项，双击或点击“详情”打开单台控制面板。
- 单台控制面板支持开关机、温度、模式、风速、上下风、左右风；控件会根据遥控器能力禁用不支持的功能。
- 批量开机/关机、批量设置、停止后续批量任务。
- 重命名、复制、删除空调。
- 38K/40K 红外测试。
- 串口日志弹窗查看 TX/RX/USB/ERR 记录，并支持复制和清空。

Android 构建：

```powershell
cd android_app
.\package_android_apk.ps1
```

脚本会运行 `assembleDebug`，并把 APK 复制到 `release/Android_apk/`。首次构建需要 Android Studio/Android SDK，并会下载 Gradle/Android/Kotlin 依赖。
`android_app/local.properties` 保存本机 SDK 路径，属于本地配置，不提交到仓库；如果 Gradle 找不到 SDK，可在该文件中写入 `sdk.dir=C\:\\Users\\<用户名>\\AppData\\Local\\Android\\Sdk`。

## 固件

V1.1 当前固件目标芯片为 ESP32-S3，串口仍使用 UART0 / USB-UART 桥接方式，不启用 USB CDC 作为主控制串口。

红外发射引脚默认是 GPIO4，配置在：

```text
airconditioner_control_main/main/core/ac_config.h
```

串口参数保持不变：

```text
115200 8N1
每条命令以 \n 结尾
```

常用命令：

```text
PING
CATALOG
HELP
IRTEST freq=38000 mode=nec count=4 duty=33
IRTEST freq=40000 mode=nec count=4 duty=33
AC remote=midea_standard power=1 mode=cool temp=26 fan=auto swingv=off swingh=off
AC remote=midea_rn02s13 action=temp power=1 mode=cool temp=27 fan=auto swingv=off swingh=off
```

`CATALOG` 会输出当前固件内置的品牌和遥控器候选。当前版本只暴露 IRremoteESP8266/IRac 能直接发送的品牌，已包含：美的、格力、海尔、TCL、科龙、大金、日立、松下、三菱电机、三菱重工、东芝、夏普、三星、LG、开利、奥克斯、Airton、Airwell、Amcor、Argo、博世、Coolix、Corona、德龙、Ecoclim、Eurom、富士通、Goodweather、Kelvinator、Mirage、Neoclima、Rhoss、三洋、Teknopoint、Technibel、TECO、Trotec、Truma、Vestel、Voltas、惠而浦、Transcold。

`IRTEST` 用来排查硬件发射链路。默认发送 NEC 测试帧，普通红外接收模块更容易识别；如果要用示波器看连续载波，可以使用：

```text
IRTEST freq=38000 mode=carrier ms=300 count=2
```

注意：常见红外接收头是解调接收器，不适合直接判断连续载波；用 NEC 测试帧更可靠。

## Qt 测试上位机

Qt 工程位于：

```text
qt_host/
```

v1.1 分支会保留 Qt 上位机，方便在电脑上完整测试串口协议、红外发射链路和本地空调库。它仍然是测试/参考实现，真正产品端可以换成 Android，只要按 `docs/android_serial_protocol.md` 发送同样的文本命令即可。

运行环境：

- Qt6 Widgets
- Qt6 SerialPort
- CMake + Ninja
- Windows 下推荐使用 MSYS2 UCRT64 的 Qt6；构建和运行前把 `C:\msys64\ucrt64\bin` 放到 `PATH` 前面，避免 `moc.exe` 或运行时 DLL 找不到。

Qt 构建：

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
qt-cmake.bat -S qt_host -B qt_host/build -G Ninja
cmake --build qt_host/build
```

构建完成后运行：

```powershell
.\qt_host\build\universal_ac_host.exe
```

主界面包含串口连接区、可选空调、开启空调、关闭所有空调、添加空调、重命名、删除空调、批量设置、停止任务、详情控制、红外测试和串口日志入口。已保存空调以灰蓝背景上的灰色卡片网格展示，卡片内用浅蓝边框分割信息格，可以直接看到名称、开关状态、温度、模式、风速、上下风、左右风和遥控器信息。窗口足够宽时一行最多显示 8 张卡片，缩小时自动减少列数，最小保持 2 张卡片宽度。

卡片支持勾选多选、右键菜单、搜索和排序。左键单击设置当前焦点，双击打开详情控制，右键菜单可进入详情、重命名、复制和删除；顶部“重命名”按钮会修改当前焦点空调名称。有勾选时，“开启空调”“关闭所有空调”“删除空调”只作用于已选空调；没有勾选时，开关按钮仍按原逻辑作用于全部空调，删除按钮删除当前焦点卡片。批量设置需要先勾选空调，会按约 `1500ms` 间隔逐台发送完整状态命令。串口 TX/RX 日志默认不占用主界面，点击“串口日志”后弹窗查看，并可清空或复制。

Qt 上位机连接 ESP32 后会自动发送 `PING` 心跳。默认每 3 秒检测一次，如果连续两次没有收到 `OK PONG`，或 QtSerialPort 报告 USB/串口错误，界面会判定 USB 已断开并弹出“USB已断开，请重新连接。”提示。点击弹窗里的“重新连接”会优先尝试重新打开断开前的 `COMx`，成功后自动重新发送 `PING` 和 `CATALOG`。

推荐测试流程：

1. 先烧录并启动 ESP32 固件，确认串口波特率是 `115200`。
2. 打开 Qt 上位机，选择 ESP32 对应的 `COMx`，点击连接。
3. 连接后 Qt 会发送 `PING` 并读取 `CATALOG`，用固件返回的目录刷新品牌和候选遥控器。
4. 点击“添加空调”，选择品牌，按候选遥控器逐个测试开机和调温；确认成功后保存到本地空调库。
5. 在空调卡片上单击选中，双击进入详情控制，可控制开关机、温度、模式、风速、上下风、左右风。
6. “开启空调”和“关闭所有空调”会遍历本地空调库并按约 `200ms` 间隔逐台发送 `action=power` 命令；ESP32 固件会同步执行红外发送，串口缓冲负责承接短时间内到达的命令。
7. 断开 USB 线测试心跳保护：Qt 应弹出断线提示，重插后点击“重新连接”恢复串口通信。
8. 点击“红外测试”后选择 38K 或 40K，会发送 `IRTEST`，用于检查 GPIO4、红外 LED、载波频率和接收头是否能看到信号。

已发现并确认可用的空调保存在电脑端 `discovered_ac.json`。这份 JSON 只是 Qt 测试工具自己的本地库，Android 端可以用自己的数据库或配置文件保存 `remoteId` 和空调状态。

## 构建

ESP-IDF：

```powershell
. C:\esp\v5.5.3\esp-idf\export.ps1
idf.py -C airconditioner_control_main build
idf.py -C airconditioner_control_main -p COMx flash monitor
```

也可以先进入固件目录再构建：

```powershell
cd airconditioner_control_main
idf.py build
idf.py -p COMx flash monitor
```

仓库根目录提供了一个 ESP-IDF 兼容入口，但推荐始终使用 `-C airconditioner_control_main`，避免把构建产物放错位置。

Qt：

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
qt-cmake.bat -S qt_host -B qt_host/build -G Ninja
cmake --build qt_host/build
```

如果 Qt 找不到串口模块，需要安装 Qt6 SerialPort 模块，例如 MSYS2 UCRT64 环境中的 `mingw-w64-ucrt-x86_64-qt6-serialport`。
