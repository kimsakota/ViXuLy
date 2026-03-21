#include "frame.h"
#include "../drivers/uart.h"
#include <stdint.h>

static uint8_t state = 0;
static uint8_t buffer[16];
static uint8_t index = 0;
static uint8_t expected_len = 0;

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
            // Kiểm tra byte đầu tiên có phải là FRAME_HEADER không, nếu có thì bắt đầu
            // xây dựng khung dữ liệu
            if (byte == FRAME_HEADER) {
                index = 0;
                buffer[index++] = byte;
                state = 1;
            }
            break;
        case 1:
            // Reading cmd, lưu byte vào buffer và chuyển sang trạng thái tiếp theo để
            // đọc len
            buffer[index++] = byte;
            state = 2;
            break;
        case 2: // Reading len, lưu byte vào buffer và xác định độ dài dữ liệu mong
            // đợi
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