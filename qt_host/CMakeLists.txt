cmake_minimum_required(VERSION 3.21)

project(UniversalAcHost LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort)

qt_add_executable(universal_ac_host
    src/ac_catalog.cpp
    src/ac_catalog.h
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
    set_target_properties(universal_ac_host PROPERTIES WIN32_EXECUTABLE ON)
endif()
