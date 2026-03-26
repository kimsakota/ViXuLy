#include "frame.h"
#include "../drivers/uart.h"
#include <stdint.h>

static uint8_t state = 0; // Lưu trạng thái hiện tại của quá trình phân tích khung dữ liệu
static uint8_t buffer[16]; // Bộ đệm tạm thời để lưu trữ các byte của khung dữ liệu đang được phân tích
static uint8_t index = 0; // Chỉ số hiện tại trong bộ đệm, dùng để theo dõi số byte đã nhận được cho khung dữ liệu hiện tại
static uint8_t expected_len = 0; // Độ dài dữ liệu mong đợi, được xác định khi nhận được byte độ dài (len) trong khung dữ liệu

void frame_init() {
    state = 0;
    index = 0;
}

uint8_t frame_calculate_checksum(uint8_t cmd, uint8_t len, uint8_t* data) {
    uint8_t checksum = FRAME_HEADER ^ cmd ^ len;

    for (uint8_t i = 0; i < len; i++)
        checksum ^= data[i];

    return checksum;
}

bool frame_parse_byte(uint8_t byte, frame_t* out) {
    switch (state) {
        case 0:
            // Check if first byte is FRAME_HEADER, and if so start building data frame
            if (byte == FRAME_HEADER) {
                index = 0;
                buffer[index++] = byte;
                state = 1;
            }
            break;
        case 1:
            // Reading cmd, cache byte into buffer and switch state to len
            buffer[index++] = byte;
            state = 2;
            break;
        case 2: // Reading len, set expected data length
            buffer[index++] = byte;
            expected_len = byte;

            if (expected_len > FRAME_MAX_DATA)
                state = 0; // Invalid length, reset
            else if (expected_len == 0)
                state = 4; // No data, go to checksum
            else
                state = 3; // Expecting data
            break;
        case 3: // Reading data
            buffer[index++] = byte;
            // index = 4
            if (index == (3 + expected_len))
                state = 4; // All data received, go to checksum
            else if (index > (3 + expected_len))
				state = 0; // Too much data, reset
            break;
        case 4: { // Reading checksum
            uint8_t cmd = buffer[1];
            uint8_t len = buffer[2];

            uint8_t* data = &buffer[3];

            uint8_t cs = frame_calculate_checksum(cmd, len, data);

            if (cs == byte) {
                out->cmd = cmd;
                out->len = len;

                for (uint8_t i = 0; i < len; i++)
                    out->data[i] = data[i];

                state = 0;   // Reset for next frame
                return true; // Frame complete
            }
            else
                state = 0;
        }
    }
    return false; // Frame not complete yet
}

void frame_send(uint8_t cmd, uint8_t len, uint8_t* data)
{
    uint8_t cs = frame_calculate_checksum(cmd, len, data);
    uart_write_char(FRAME_HEADER);
    uart_write_char(cmd);
    uart_write_char(len);
    for (uint8_t i = 0; i < len; i++)
        uart_write_char(data[i]);
    uart_write_char(cs);
}