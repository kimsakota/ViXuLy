#include "app.h"
#include "../config/system_config.h"
#include "../core/memory.h"
#include "../drivers/ppi8255.h"
#include "../drivers/uart.h"
#include "../protocol/command.h"
#include "../protocol/frame.h"
#include "../services/device_service.h"
#include "../services/measurement_service.h"
#include <util/delay.h>


void app_init() {
  uart_init();
  ppi8255_init();
  memory_init();
  device_service_init();
  measurement_service_init();
  frame_init();

  uart_write_string("System Ready\r\n");
}

void app_task() {
  uint8_t byte;
  frame_t frame;

  // ── Poll nhận CMD từ PC trong ~485ms ─────────────────────────────────
  // Sử dụng delay nhỏ để tăng tần suất lấy mẫu UART bit-banging, không bị miss start bit (104us)
  // Mỗi loop không có dữ liệu tốn ~10us + overhead ~2us. 48500 iterations tương đương ~580ms.
  // Nếu có dữ liệu, đọc tốn ~1ms.
  for (uint16_t t = 0; t < 48500; t++) {
    if (uart_read_char((char *)&byte)) {
      if (frame_parse_byte(byte, &frame)) {
        if (frame.cmd == CMD_SET_ALL && frame.len == 1) {
          device_service_set_all(frame.data[0]);
          uart_write_string("ACK\r\n");
        } else if (frame.cmd == CMD_SET_SINGLE && frame.len == 2) {
          uint8_t index = frame.data[0];
          uint8_t value = frame.data[1];
          if (value)
            device_service_turn_on(index);
          else
            device_service_turn_off(index);
          uart_write_string("ACK\r\n");
          } else if (frame.cmd == CMD_READ_SENSOR && frame.len == 0) {
            uint8_t payload[9];
            for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
              payload[i] = measurement_service_read(i);
            }
            payload[8] = device_service_get_state();
            frame_send(CMD_SENSOR_DATA, 9, payload);
            uart_write_string("ACK\r\n");
        } else {
          uart_write_string("NACK\r\n");
        }
      }
      // Byte vừa đọc xong ~1ms, không cần delay thêm
    } else {
      _delay_us(10); // Không có byte: chờ 10us trước khi check lại để lấy mẫu Start Bit chính xác
    }
  }

  // ── Push sensor data 1 lần sau 485ms (~14ms) ─────────────────────────
  uint8_t payload[9];
  uint8_t device_state = device_service_get_state();
  for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
    // Only read ADC for devices that are ON.
    // Channels with no connected sensor (floating 74HC4051 inputs) pick up
    // crosstalk from active channels in Proteus simulation, so gating by
    // device state prevents false current readings on inactive channels.
    payload[i] = (device_state & (1 << i)) ? measurement_service_read(i) : 0;
  }
  payload[8] = device_state;
  // Frame: [AA][04][09][current0..current7][device_state][CS] = 13 bytes
  frame_send(CMD_SENSOR_DATA, 9, payload);
}