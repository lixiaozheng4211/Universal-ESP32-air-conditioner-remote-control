# Universal ESP32 空调遥控器

这个仓库包含两部分：

- `midea_ac_idf/`：ESP32 固件，通过串口接收统一文本命令，并从美的、格力、海尔等候选遥控器中选择红外协议发送。
- `qt_host/`：Qt6 Widgets 上位机，用串口连接 ESP32，支持添加空调、遍历已保存空调并发送开机命令。

旧的 ESP-IDF NEC 示例、重复的 `gree_ac_idf/` 工程、顶层 BSP 示例和构建产物已经清理掉。当前格力支持集中在 `midea_ac_idf/main/remotes/gree_remotes.cpp`。

## 固件

红外发射引脚默认是 GPIO4，配置在：

```text
midea_ac_idf/main/ac_config.h
```

常用命令：

```text
PING
CATALOG
HELP
IRTEST freq=38000 mode=nec count=4 duty=33
IRTEST freq=40000 mode=nec count=4 duty=33
AC remote=midea_standard power=1 mode=cool temp=26 fan=auto swingv=off swingh=off eco=0
```

`IRTEST` 用来排查硬件发射链路。默认发送 NEC 测试帧，普通红外接收模块更容易识别；如果要用示波器看连续载波，可以使用：

```text
IRTEST freq=38000 mode=carrier ms=300 count=2
```

注意：常见红外接收头是解调接收器，不适合直接判断连续载波；用 NEC 测试帧更可靠。

## Qt 上位机

Qt 工程位于：

```text
qt_host/
```

主界面包含串口连接区、开启空调、添加空调、38K 测试、40K 测试。38K/40K 测试按钮会分别发送：

```text
IRTEST freq=38000 mode=nec count=4 duty=33
IRTEST freq=40000 mode=nec count=4 duty=33
```

已发现并确认可用的空调保存在电脑端 `discovered_ac.json`。

## 构建

ESP-IDF：

```powershell
. C:\esp\v5.5.3\esp-idf\export.ps1
idf.py -C midea_ac_idf build
idf.py -C midea_ac_idf -p COMx flash monitor
```

也可以先进入固件目录再构建：

```powershell
cd midea_ac_idf
idf.py build
idf.py -p COMx flash monitor
```

不要在仓库根目录直接运行 `idf.py build`。仓库根目录只是总目录，ESP-IDF 工程根目录是 `midea_ac_idf/`。

Qt：

```powershell
qt-cmake.bat -S qt_host -B qt_host/build -G Ninja
cmake --build qt_host/build
```

如果 Qt 找不到串口模块，需要安装 Qt6 SerialPort 模块。
