# Universal ESP32 空调串口遥控器

这个仓库实现了一个“串口空调遥控器”：上位设备通过 USB 串口给 ESP32 发送文本命令，ESP32 再选择对应空调遥控器协议并发射红外。

当前仓库包含两部分：

- `airconditioner_control_main/`：ESP32 固件主工程，负责串口协议解析、遥控器目录、IRremoteESP8266 红外发送。
- `qt_host/`：Qt6 Widgets 测试/参考上位机，用来在电脑上验证串口协议、添加空调和测试红外硬件。

Qt 上位机只是调试工具，不是最终平台限制。Android、Windows、Linux 或其它主控只要能打开串口并发送同样的文本命令，就可以控制 ESP32 发射空调红外。

Android 端开发请优先阅读：

```text
docs/android_serial_protocol.md
```

## 固件

红外发射引脚默认是 GPIO4，配置在：

```text
airconditioner_control_main/main/core/ac_config.h
```

串口参数：

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

`CATALOG` 会输出当前固件内置的品牌和遥控器候选。当前版本暴露 IRremoteESP8266/IRac 能直接发送的品牌：美的、格力、海尔、TCL、科龙、大金、日立、松下、三菱电机、三菱重工、东芝、夏普、三星、LG、开利、奥克斯。

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

v1.0 分支会保留 Qt 上位机，方便在电脑上完整测试串口协议和红外发射链路。它仍然是测试/参考实现，真正产品端可以换成 Android，只要按 `docs/android_serial_protocol.md` 发送同样的文本命令即可。

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

主界面包含串口连接区、开启空调、添加空调、删除空调、详情控制、38K 测试、40K 测试。推荐测试流程：

1. 先烧录并启动 ESP32 固件，确认串口波特率是 `115200`。
2. 打开 Qt 上位机，选择 ESP32 对应的 `COMx`，点击连接。
3. 连接后 Qt 会发送 `PING` 并读取 `CATALOG`，用固件返回的目录刷新品牌和候选遥控器。
4. 点击“添加空调”，选择品牌，按候选遥控器逐个测试开机和调温；确认成功后保存到本地空调库。
5. 在空调库中点击某台空调进入详情控制，可控制开关机、温度、模式、风速、上下风、左右风。
6. “开启空调”会遍历本地空调库并按约 `1500ms` 间隔逐台发送开机命令，避免红外命令挤在一起。
7. 38K/40K 测试按钮会发送 `IRTEST`，用于检查 GPIO4、红外 LED、载波频率和接收头是否能看到信号。

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
