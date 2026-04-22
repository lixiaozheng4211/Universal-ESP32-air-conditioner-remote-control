#include "ir_nec_encoder.h"
#include "esp_check.h"

static const char *TAG = "nec_encoder";

/**
 * @brief NEC 编码器的内部结构体
 *
 * 我们通过“面向对象”的思想，继承了标准的 RMT 编码器接口 (`rmt_encoder_t`)。
 */
typedef struct {
  rmt_encoder_t
      base; // 基础“类”，声明了标准编码器所需的接口 (encode, reset, del)
  // NEC 协议有特殊的引导码 (Leading code) 和结束码 (Ending code)。
  // 我们使用 RMT 自带的 copy_encoder 来直接复制这些固定的电平符号。
  // 下面是子类的拓展属性 !!!
  rmt_encoder_t *copy_encoder;
  // 对于地址和命令数据，我们需要把字节(0和1)转换成对应的 RMT 符号。
  // 我们使用 RMT 自带的 bytes_encoder 来完成这个转换。
  rmt_encoder_t *bytes_encoder;
  rmt_symbol_word_t
      nec_leading_symbol; // 保存 NEC 引导码的 RMT 符号表示 (9ms高 + 4.5ms低)
  rmt_symbol_word_t
      nec_ending_symbol; // 保存 NEC 结束码的 RMT 符号表示 (560us高 + 结束)
  int state;             // 状态机的当前状态，用于在编码过程中记录进行到了哪一步
                         // (引导码->地址->命令->结束码
} rmt_ir_nec_encoder_t;

/**
 * @brief 核心编码函数：将 NEC 数据转换成 RMT 符号 (硬件回调函数)
 *
 * 这是一个被底层 RMT 驱动自动回调的函数。当我们调用 `rmt_transmit()` 时，
 * 硬件驱动会反复调用这个函数，向我们要 RMT
 * 符号（波形数据），直到我们告诉它全部完成。
 *
 * @note RMT_ENCODER_FUNC_ATTR：
 *       这是一个特殊的宏。它的作用是把这个函数强制放到 ESP32 的高速内部内存
 * (IRAM) 中，而不是 Flash 中。 因为这个函数极有可能会在底层硬件中断 (ISR)
 * 里被快速调用。如果放在 Flash 里，读取太慢甚至会导致系统崩溃。
 *
 * @param[in] encoder      通用的编码器基类指针。底层只认识它，我们需要用
 * __containerof 把它转回 NEC 编码器。
 * @param[in] channel      RMT
 * 的发射通道句柄。我们不需要直接操作它，只负责把它“透传”给底层的 copy/bytes
 * 编码器。
 * @param[in] primary_data 也就是你在 `rmt_transmit()`
 * 里传入的那个你要发送的包裹（`scan_code` 结构体的地址）。
 * @param[in] data_size
 * 包裹的大小（`sizeof(scan_code)`）。在这个简单的驱动里其实用不到，因为数据大小是固定的。
 * @param[out] ret_state
 * 【关键输出参数】用来给底层驱动“打报告”，告诉它当前进度：
 *                         - 如果报
 * `RMT_ENCODING_MEM_FULL`：意思是“大哥，你给我的硬件内存被我填满了，你先去发射，发空了再来叫我”。
 *                         - 如果报
 * `RMT_ENCODING_COMPLETE`：意思是“我把引导码、数据、结束码全翻译完了，这次任务彻底结束”。
 *
 * @return size_t 本次调用期间，我们成功向 RMT 通道写入了多少个 RMT 波形符号。
 */
RMT_ENCODER_FUNC_ATTR
static size_t rmt_encode_ir_nec(rmt_encoder_t *encoder,
                                rmt_channel_handle_t channel,
                                const void *primary_data, size_t data_size,
                                rmt_encode_state_t *ret_state) {
  // 通过 base 指针获取到我们自定义的 rmt_ir_nec_encoder_t 结构体指针
  rmt_ir_nec_encoder_t *nec_encoder =
      __containerof(encoder, rmt_ir_nec_encoder_t, base);

  // session_state 用于接收“小弟”（内部的 copy/bytes 编码器）单次干活的进度汇报
  rmt_encode_state_t session_state = RMT_ENCODING_RESET;
  // state 用于记录“老板”（本 NEC 编码器整体）的进度，最终会通过参数 ret_state
  // 向上级驱动汇报
  rmt_encode_state_t state = RMT_ENCODING_RESET;
  // 记录本次函数调用期间，总共向硬件通道里塞了多少个波形符号（作为函数的返回值）
  size_t encoded_symbols = 0;
  // 将底层“快递员”送来的 void * 类型的盲盒包裹，拆箱强转回我们认识的 NEC
  // 数据包，方便后面读取 address 和 command
  ir_nec_scan_code_t *scan_code = (ir_nec_scan_code_t *)primary_data;
  // 叫出负责“死板复印波形”的一号小弟（专门用来发引导码和结束码）
  rmt_encoder_handle_t copy_encoder = nec_encoder->copy_encoder;
  // 叫出负责“把0/1数字翻译成波形”的二号小弟（专门用来发地址和命令数据）
  rmt_encoder_handle_t bytes_encoder = nec_encoder->bytes_encoder;

  // 状态机：按照 NEC 协议的顺序逐步编码
  switch (nec_encoder->state) {
  case 0: // 状态 0：发送引导码 (Leading code)
    // 使用 copy_encoder 将 nec_leading_symbol 写入 RMT 通道
    encoded_symbols += copy_encoder->encode(
        copy_encoder,                     // 传递编码器指针
        channel,                          // 传递 RMT 通道句柄
        &nec_encoder->nec_leading_symbol, // 要发送的波形数据 (NEC 引导码)
        sizeof(rmt_symbol_word_t),        // 数据大小
        &session_state);
    if (session_state & RMT_ENCODING_COMPLETE) {
      nec_encoder->state = 1; // 当前阶段完成，切换到下一个状态 (发地址)
    }
    if (session_state & RMT_ENCODING_MEM_FULL) {
      state |= RMT_ENCODING_MEM_FULL;
      goto out; // 如果 RMT
                // 内存满了，暂停编码并返回，等待硬件发送空出内存后再次被调用
    }
    /* fall-through */

  case 1: // 状态 1：发送地址 (Address)
    // 使用 bytes_encoder 将 16位 的 address 数据转换成 RMT 符号并写入通道
    encoded_symbols +=
        bytes_encoder->encode(bytes_encoder, channel, &scan_code->address,
                              sizeof(uint16_t), &session_state);
    if (session_state & RMT_ENCODING_COMPLETE) {
      nec_encoder->state = 2; // 当前阶段完成，切换到下一个状态 (发命令)
    }
    if (session_state & RMT_ENCODING_MEM_FULL) {
      state |= RMT_ENCODING_MEM_FULL;
      goto out; // 内存满，暂停退出
    }
    /* fall-through */

  case 2: // 状态 2：发送命令 (Command)
    // 使用 bytes_encoder 将 16位 的 command 数据转换成 RMT 符号并写入通道
    encoded_symbols +=
        bytes_encoder->encode(bytes_encoder, channel, &scan_code->command,
                              sizeof(uint16_t), &session_state);
    if (session_state & RMT_ENCODING_COMPLETE) {
      nec_encoder->state = 3; // 当前阶段完成，切换到下一个状态 (发结束码)
    }
    if (session_state & RMT_ENCODING_MEM_FULL) {
      state |= RMT_ENCODING_MEM_FULL;
      goto out; // 内存满，暂停退出
    }
    /* fall-through */

  case 3: // 状态 3：发送结束码 (Ending code)
    // 使用 copy_encoder 将 nec_ending_symbol 写入 RMT 通道
    encoded_symbols += copy_encoder->encode(
        copy_encoder, channel, &nec_encoder->nec_ending_symbol,
        sizeof(rmt_symbol_word_t), &session_state);
    if (session_state & RMT_ENCODING_COMPLETE) {
      nec_encoder->state =
          RMT_ENCODING_RESET; // 所有状态都完成了，复位状态机，准备下一次发送
      state |=
          RMT_ENCODING_COMPLETE; // 告诉底层驱动：整个 NEC 帧已经全部编码完毕
    }
    if (session_state & RMT_ENCODING_MEM_FULL) {
      state |= RMT_ENCODING_MEM_FULL;
      goto out; // 内存满，暂停退出
    }
  }
out:
  *ret_state = state; // 返回当前的整体状态
  return encoded_symbols;
}

/**
 * @brief 删除/释放 NEC 编码器
 */
static esp_err_t rmt_del_ir_nec_encoder(rmt_encoder_t *encoder) {
  rmt_ir_nec_encoder_t *nec_encoder =
      __containerof(encoder, rmt_ir_nec_encoder_t, base);
  // 释放内部的两个子编码器
  rmt_del_encoder(nec_encoder->copy_encoder);
  rmt_del_encoder(nec_encoder->bytes_encoder);
  // 释放自己
  free(nec_encoder);
  return ESP_OK;
}

/**
 * @brief 复位 NEC 编码器
 */
RMT_ENCODER_FUNC_ATTR
static esp_err_t rmt_ir_nec_encoder_reset(rmt_encoder_t *encoder) {
  rmt_ir_nec_encoder_t *nec_encoder =
      __containerof(encoder, rmt_ir_nec_encoder_t, base);
  // 复位内部的两个子编码器
  rmt_encoder_reset(nec_encoder->copy_encoder);
  rmt_encoder_reset(nec_encoder->bytes_encoder);
  // 状态机归零
  nec_encoder->state = RMT_ENCODING_RESET;
  return ESP_OK;
}

/**
 * @brief 创建一个支持 NEC 协议的 RMT 编码器
 */
esp_err_t rmt_new_ir_nec_encoder(const ir_nec_encoder_config_t *config,
                                 rmt_encoder_handle_t *ret_encoder) {
  esp_err_t ret = ESP_OK;
  rmt_ir_nec_encoder_t *nec_encoder = NULL;

  // 检查传入参数是否合法
  ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG,
                    "invalid argument");

  // 为自定义编码器结构体分配内存
  nec_encoder = rmt_alloc_encoder_mem(sizeof(rmt_ir_nec_encoder_t));
  ESP_GOTO_ON_FALSE(nec_encoder, ESP_ERR_NO_MEM, err, TAG,
                    "no mem for ir nec encoder");

  // 绑定接口回调函数
  nec_encoder->base.encode = rmt_encode_ir_nec;
  nec_encoder->base.del = rmt_del_ir_nec_encoder;
  nec_encoder->base.reset = rmt_ir_nec_encoder_reset;

  // 1. 创建子编码器：copy_encoder (用于原样复制引导码和结束码)
  rmt_copy_encoder_config_t copy_encoder_config = {};
  ESP_GOTO_ON_ERROR(
      rmt_new_copy_encoder(&copy_encoder_config, &nec_encoder->copy_encoder),
      err, TAG, "create copy encoder failed");

  // 根据配置的分辨率，构造 NEC 的引导码 (9ms 脉冲 + 4.5ms 空闲)
  nec_encoder->nec_leading_symbol = (rmt_symbol_word_t){
      .level0 = 1,                                         // 高电平
      .duration0 = 9000ULL * config->resolution / 1000000, // 持续 9000us (9ms)
      .level1 = 0,                                         // 低电平
      .duration1 =
          4500ULL * config->resolution / 1000000, // 持续 4500us (4.5ms)
  };
  // 构造 NEC 的结束码 (560us 脉冲 + 结束)
  nec_encoder->nec_ending_symbol = (rmt_symbol_word_t){
      .level0 = 1,                                     // 高电平
      .duration0 = 560 * config->resolution / 1000000, // 持续 560us
      .level1 = 0,                                     // 低电平
      .duration1 = 0x7FFF,                             // 持续最大时间，表示结束
  };

  // 2. 创建子编码器：bytes_encoder (用于将 0 和 1 的二进制数据转换为电平符号)
  rmt_bytes_encoder_config_t bytes_encoder_config = {
      // 定义逻辑 '0' 对应的电平符号 (560us 高电平 + 560us 低电平)
      .bit0 =
          {
              .level0 = 1,
              .duration0 = 560 * config->resolution / 1000000, // T0H=560us
              .level1 = 0,
              .duration1 = 560 * config->resolution / 1000000, // T0L=560us
          },
      // 定义逻辑 '1' 对应的电平符号 (560us 高电平 + 1690us 低电平)
      .bit1 =
          {
              .level0 = 1,
              .duration0 = 560 * config->resolution / 1000000, // T1H=560us
              .level1 = 0,
              .duration1 = 1690 * config->resolution / 1000000, // T1L=1690us
          },
  };
  ESP_GOTO_ON_ERROR(
      rmt_new_bytes_encoder(&bytes_encoder_config, &nec_encoder->bytes_encoder),
      err, TAG, "create bytes encoder failed");

  // 返回创建好的编码器基类指针
  *ret_encoder = &nec_encoder->base;
  return ESP_OK;

err:
  // 如果中间任何一步失败，走到这里清理已分配的内存，防止内存泄漏
  if (nec_encoder) {
    if (nec_encoder->bytes_encoder) {
      rmt_del_encoder(nec_encoder->bytes_encoder);
    }
    if (nec_encoder->copy_encoder) {
      rmt_del_encoder(nec_encoder->copy_encoder);
    }
    free(nec_encoder);
  }
  return ret;
}
