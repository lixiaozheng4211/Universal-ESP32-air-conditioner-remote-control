/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "driver/rmt_rx.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "ir_nec_encoder.h"

// RMT 的基础时钟分辨率，1MHz代表精度为1微秒 (1 tick = 1us)
#define EXAMPLE_IR_RESOLUTION_HZ 1000000 
#define EXAMPLE_IR_TX_GPIO_NUM 18 // 红外发射(TX)连接的 GPIO 引脚号
#define EXAMPLE_IR_RX_GPIO_NUM 19 // 红外接收(RX)连接的 GPIO 引脚号
// RMT 符号解析为比特流时的误差容限 (200微秒)。因为真实环境有干扰，时间不可能完全精准。
#define EXAMPLE_IR_NEC_DECODE_MARGIN 200 

/**
 * @brief NEC 协议时间参数规范 (单位：微秒 us)
 * 
 * NEC 协议通过脉冲的宽度来代表数据：
 * 引导码：9ms脉冲 + 4.5ms空闲
 * 逻辑'0'：560us脉冲 + 560us空闲
 * 逻辑'1'：560us脉冲 + 1690us空闲
 * 重复码：9ms脉冲 + 2.25ms空闲
 */
#define NEC_LEADING_CODE_DURATION_0 9000 // 引导码高电平时间 9000us
#define NEC_LEADING_CODE_DURATION_1 4500 // 引导码低电平时间 4500us
#define NEC_PAYLOAD_ZERO_DURATION_0 560  // 逻辑'0'的高电平时间 560us
#define NEC_PAYLOAD_ZERO_DURATION_1 560  // 逻辑'0'的低电平时间 560us
#define NEC_PAYLOAD_ONE_DURATION_0 560   // 逻辑'1'的高电平时间 560us
#define NEC_PAYLOAD_ONE_DURATION_1 1690  // 逻辑'1'的低电平时间 1690us
#define NEC_REPEAT_CODE_DURATION_0 9000  // 重复码高电平时间 9000us
#define NEC_REPEAT_CODE_DURATION_1 2250  // 重复码低电平时间 2250us

static const char *TAG = "example";

/**
 * @brief 用于保存 NEC 解码结果的全局变量
 */
static uint16_t s_nec_code_address; // 存放解析出的地址
static uint16_t s_nec_code_command; // 存放解析出的命令

/**
 * @brief 检查一个信号的时间长度是否在预期的范围内 (考虑了误差容限)
 */
static inline bool nec_check_in_range(uint32_t signal_duration,
                                      uint32_t spec_duration) {
  return (signal_duration < (spec_duration + EXAMPLE_IR_NEC_DECODE_MARGIN)) &&
         (signal_duration > (spec_duration - EXAMPLE_IR_NEC_DECODE_MARGIN));
}

/**
 * @brief 判断 RMT 符号是否代表 NEC 的逻辑 '0'
 * 
 * 检查符号的两个电平持续时间是否符合 逻辑 '0' 的特征：560us + 560us
 */
static bool nec_parse_logic0(rmt_symbol_word_t *rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_PAYLOAD_ZERO_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_PAYLOAD_ZERO_DURATION_1);
}

/**
 * @brief 判断 RMT 符号是否代表 NEC 的逻辑 '1'
 * 
 * 检查符号的两个电平持续时间是否符合 逻辑 '1' 的特征：560us + 1690us
 */
static bool nec_parse_logic1(rmt_symbol_word_t *rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_PAYLOAD_ONE_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_PAYLOAD_ONE_DURATION_1);
}

/**
 * @brief 将接收到的 RMT 符号流解码为 NEC 的地址和命令
 * 
 * @param rmt_nec_symbols 硬件接收到的波形符号数组
 * @return true 表示解码成功，false 表示数据不符合NEC协议
 */
static bool nec_parse_frame(rmt_symbol_word_t *rmt_nec_symbols) {
  rmt_symbol_word_t *cur = rmt_nec_symbols;
  uint16_t address = 0;
  uint16_t command = 0;
  
  // 1. 检查第一个符号是不是合法的引导码 (9ms + 4.5ms)
  bool valid_leading_code =
      nec_check_in_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) &&
      nec_check_in_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);
  if (!valid_leading_code) {
    return false;
  }
  cur++; // 引导码正确，移动到下一个符号

  // 2. 解析 16 位的地址数据
  for (int i = 0; i < 16; i++) {
    if (nec_parse_logic1(cur)) {
      address |= 1 << i; // 如果是逻辑1，把该位设为1
    } else if (nec_parse_logic0(cur)) {
      address &= ~(1 << i); // 如果是逻辑0，把该位设为0
    } else {
      return false; // 既不是0也不是1，波形错误
    }
    cur++;
  }
  
  // 3. 解析 16 位的命令数据
  for (int i = 0; i < 16; i++) {
    if (nec_parse_logic1(cur)) {
      command |= 1 << i;
    } else if (nec_parse_logic0(cur)) {
      command &= ~(1 << i);
    } else {
      return false;
    }
    cur++;
  }
  
  // 4. 保存解析结果到全局变量
  s_nec_code_address = address;
  s_nec_code_command = command;
  return true;
}

/**
 * @brief 检查 RMT 符号是否代表 NEC 的“重复码” (按住遥控器不放时发送)
 */
static bool nec_parse_frame_repeat(rmt_symbol_word_t *rmt_nec_symbols) {
  return nec_check_in_range(rmt_nec_symbols->duration0,
                            NEC_REPEAT_CODE_DURATION_0) &&
         nec_check_in_range(rmt_nec_symbols->duration1,
                            NEC_REPEAT_CODE_DURATION_1);
}

/**
 * @brief 解码 RMT 符号并打印结果 (测试辅助函数)
 */
static void example_parse_nec_frame(rmt_symbol_word_t *rmt_nec_symbols,
                                    size_t symbol_num) {
  printf("NEC frame start---\r\n");
  for (size_t i = 0; i < symbol_num; i++) {
    printf("{%d:%d},{%d:%d}\r\n", rmt_nec_symbols[i].level0,
           rmt_nec_symbols[i].duration0, rmt_nec_symbols[i].level1,
           rmt_nec_symbols[i].duration1);
  }
  printf("---NEC frame end: ");
  // 根据收到的符号总数判断是正常帧还是重复帧
  switch (symbol_num) {
  case 34: // NEC 正常帧：1个引导码 + 16个地址 + 16个命令 + 1个结束码 = 34个符号
    if (nec_parse_frame(rmt_nec_symbols)) {
      printf("Address=%04X, Command=%04X\r\n\r\n", s_nec_code_address,
             s_nec_code_command);
    }
    break;
  case 2: // NEC 重复帧：1个引导码 + 1个结束码 = 2个符号
    if (nec_parse_frame_repeat(rmt_nec_symbols)) {
      printf("Address=%04X, Command=%04X, repeat\r\n\r\n", s_nec_code_address,
             s_nec_code_command);
    }
    break;
  default:
    printf("Unknown NEC frame\r\n\r\n");
    break;
  }
}

/**
 * @brief 当 RMT 接收通道成功接收到一帧数据时触发的中断回调函数
 */
static bool example_rmt_rx_done_callback(rmt_channel_handle_t channel,
                                         const rmt_rx_done_event_data_t *edata,
                                         void *user_data) {
  BaseType_t high_task_wakeup = pdFALSE;
  QueueHandle_t receive_queue = (QueueHandle_t)user_data;
  // 把接收到的数据包发送到消息队列中，通知主循环去处理
  xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
  // 如果发送到队列唤醒了更高优先级的任务，返回 true 请求上下文切换
  return high_task_wakeup == pdTRUE;
}

void app_main(void) {
  // 1. 初始化 RMT 接收 (RX) 通道
  ESP_LOGI(TAG, "create RMT RX channel");
  rmt_rx_channel_config_t rx_channel_cfg = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = EXAMPLE_IR_RESOLUTION_HZ, // 1MHz 分辨率
      .mem_block_symbols = 64, // 分配 64 个 RMT 符号的内存空间 (34个符号就够装下一帧NEC数据)
      .gpio_num = EXAMPLE_IR_RX_GPIO_NUM, // 绑定 RX 引脚
  };
  rmt_channel_handle_t rx_channel = NULL;
  ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &rx_channel));

  // 2. 注册 RX 接收完成的中断回调
  ESP_LOGI(TAG, "register RX done callback");
  QueueHandle_t receive_queue =
      xQueueCreate(1, sizeof(rmt_rx_done_event_data_t)); // 创建一个队列用于接收数据
  assert(receive_queue);
  rmt_rx_event_callbacks_t cbs = {
      .on_recv_done = example_rmt_rx_done_callback,
  };
  ESP_ERROR_CHECK(
      rmt_rx_register_event_callbacks(rx_channel, &cbs, receive_queue));

  // 3. 配置 RX 接收的时间过滤规则
  rmt_receive_config_t receive_config = {
      // 过滤杂波：最短的 NEC 信号持续 560us。如果电平变化时间少于 1250ns(1.25us)，视为噪音丢弃
      .signal_range_min_ns = 1250, 
      // 空闲判定：最长的 NEC 信号是 9000us 引导码。如果超过 12000000ns(12ms) 电平没变化，认为接收结束
      .signal_range_max_ns = 12000000, 
  };

  // 4. 初始化 RMT 发送 (TX) 通道
  ESP_LOGI(TAG, "create RMT TX channel");
  rmt_tx_channel_config_t tx_channel_cfg = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = EXAMPLE_IR_RESOLUTION_HZ,
      .mem_block_symbols = 64,
      .trans_queue_depth = 4, // 允许后台挂起的最多的发送任务数量
      .gpio_num = EXAMPLE_IR_TX_GPIO_NUM, // 绑定 TX 引脚
  };
  rmt_channel_handle_t tx_channel = NULL;
  ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));

  // 5. 给 TX 通道加上 38KHz 载波调制 (红外发射管需要 38KHz 载波才能发送得远)
  ESP_LOGI(TAG, "modulate carrier to TX channel");
  rmt_carrier_config_t carrier_cfg = {
      .duty_cycle = 0.33,    // 占空比 33%
      .frequency_hz = 38000, // 频率 38KHz
  };
  ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));

  // 6. TX 发送配置 (不循环发送)
  rmt_transmit_config_t transmit_config = {
      .loop_count = 0, // 发完一次就停，不死循环
  };

  // 7. 安装我们自定义的 IR NEC 编码器
  ESP_LOGI(TAG, "install IR NEC encoder");
  ir_nec_encoder_config_t nec_encoder_cfg = {
      .resolution = EXAMPLE_IR_RESOLUTION_HZ,
  };
  rmt_encoder_handle_t nec_encoder = NULL;
  ESP_ERROR_CHECK(rmt_new_ir_nec_encoder(&nec_encoder_cfg, &nec_encoder));

  // 8. 启用 TX 和 RX 通道
  ESP_LOGI(TAG, "enable RMT TX and RX channels");
  ESP_ERROR_CHECK(rmt_enable(tx_channel));
  ESP_ERROR_CHECK(rmt_enable(rx_channel));

  // 定义一块内存，存放收到的底层波形符号
  rmt_symbol_word_t raw_symbols[64]; 
  rmt_rx_done_event_data_t rx_data;
  
  // 开始接收，启动硬件去监听红外引脚
  ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols),
                              &receive_config));
  
  // 9. 主循环
  while (1) {
    // 阻塞等待 1 秒钟，看有没有从队列中收到红外数据
    if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(1000)) == pdPASS) {
      // 成功收到了数据！去解析它并打印出来
      example_parse_nec_frame(rx_data.received_symbols, rx_data.num_symbols);
      // 处理完之后，要重新调用 rmt_receive，让硬件再次开始接收下一波数据
      ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols),
                                  &receive_config));
    } else {
      // 1 秒钟过去了，没有收到任何红外信号 (超时)
      // 我们自己主动发射一个测试信号出去！
      const ir_nec_scan_code_t scan_code = {
          .address = 0x0440, // 随便定的地址码
          .command = 0x3003, // 随便定的命令码
      };
      // 调用底层驱动发送数据：硬件会自动调用我们的 rmt_encode_ir_nec 把结构体转换成波形发射出去
      ESP_ERROR_CHECK(rmt_transmit(tx_channel, nec_encoder, &scan_code,
                                   sizeof(scan_code), &transmit_config));
    }
  }
}
