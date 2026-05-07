cmake_minimum_required(VERSION 3.16)

# Compatibility entry for tools that open this repository root as the ESP-IDF
# project. The actual firmware sources stay in midea_ac_idf/.
set(SDKCONFIG "${CMAKE_CURRENT_LIST_DIR}/midea_ac_idf/sdkconfig")
set(SDKCONFIG_DEFAULTS "${CMAKE_CURRENT_LIST_DIR}/midea_ac_idf/sdkconfig.defaults")
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/midea_ac_idf/main"
    "${CMAKE_CURRENT_LIST_DIR}/midea_ac_idf/components/IRremoteESP8266"
    "${CMAKE_CURRENT_LIST_DIR}/midea_ac_idf/components/arduino"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(
    DEPENDENCIES_LOCK
    "${CMAKE_CURRENT_LIST_DIR}/midea_ac_idf/dependencies.lock"
)
project(universal_ac_remote)
