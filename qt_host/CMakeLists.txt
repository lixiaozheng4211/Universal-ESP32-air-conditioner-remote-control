cmake_minimum_required(VERSION 3.21)

# Qt 上位机是一个独立的桌面程序，和 ESP-IDF 固件分开构建。
# 这里使用 Qt6 Widgets 做传统桌面界面，Qt6 SerialPort 负责和 ESP32
# 通过 USB 串口交换文本命令。
project(UniversalAcHost LANGUAGES CXX)

# 当前代码使用了 std::optional、lambda、结构体默认成员等 C++17 特性。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 打开 Qt 的自动 moc 处理。只要头文件里有 Q_OBJECT，CMake 会自动生成
# Qt 元对象代码，不需要手动调用 moc。
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

# 把所有业务源码都显式列在目标里，方便 IDE 展示，也方便后续拆模块时查找。
qt_add_executable(universal_ac_host
    src/ac_catalog.cpp
    src/ac_catalog.h
    src/ac_control_dialog.cpp
    src/ac_control_dialog.h
    src/ac_discovery_wizard.cpp
    src/ac_discovery_wizard.h
    src/ac_store.cpp
    src/ac_store.h
    src/known_ac_runner.cpp
    src/known_ac_runner.h
    src/main.cpp
    src/main_window.cpp
    src/main_window.h
    src/serial_controller.cpp
    src/serial_controller.h
)

target_link_libraries(universal_ac_host
    PRIVATE
        Qt6::Widgets
        Qt6::SerialPort
)

if (WIN32)
    # Windows 下使用 GUI 子系统，双击运行时不会额外弹出一个控制台窗口。
    set_target_properties(universal_ac_host PROPERTIES WIN32_EXECUTABLE ON)
endif()
