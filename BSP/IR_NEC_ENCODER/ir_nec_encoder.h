/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "driver/rmt_encoder.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NEC红外协议扫描码结构体
 * 
 * NEC协议的标准数据帧由两部分组成：地址(Address)和命令(Command)。
 * 通常情况下，实际发送的数据是 8位地址 + 8位地址反码 + 8位命令 + 8位命令反码，共32位。
 * 但为了简化处理，这里我们将地址和其反码组合成一个16位的 address，命令及其反码组合成16位的 command。
 */
typedef struct {
  uint16_t address; /*!< 16位地址码 (通常是 8位地址 + 8位地址反码) */
  uint16_t command; /*!< 16位命令码 (通常是 8位命令 + 8位命令反码) */
} ir_nec_scan_code_t;

/**
 * @brief NEC红外编码器配置结构体
 * 
 * 用于初始化编码器时的配置参数。
 */
typedef struct {
  uint32_t resolution; /*!< RMT编码器的分辨率，单位是Hz (例如 1000000 表示 1MHz，即1微秒的精度) */
} ir_nec_encoder_config_t;

/**
 * @brief 创建一个 RMT 编码器，用于将 NEC 帧编码为 RMT 符号
 *
 * 这个函数会分配内存并初始化一个支持 NEC 协议的编码器实例。
 * 它可以将 `ir_nec_scan_code_t` 数据结构转换成 ESP32 RMT 硬件能看懂的底层电平符号。
 *
 * @param[in] config 编码器的配置参数 (主要是时间分辨率)
 * @param[out] ret_encoder 返回的编码器句柄 (用于后续的发送操作)
 * @return
 *      - ESP_ERR_INVALID_ARG: 参数错误 (比如传入了空指针)
 *      - ESP_ERR_NO_MEM: 内存不足，无法创建编码器
 *      - ESP_OK: 编码器创建成功
 */
esp_err_t rmt_new_ir_nec_encoder(const ir_nec_encoder_config_t *config,
                                 rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif
