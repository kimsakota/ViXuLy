#include "app.h"
#include "../config/system_config.h"
#include "../drivers/ppi8255.h"
#include "../drivers/uart.h"
#include "../services/device_service.h"
#include "../protocol/frame.h"
#include "../protocol/command.h"
#include <util/delay.h>

void app_init() {
    uart_init();
    ppi8255_init();
    device_service_init();
    frame_init();

    uart_write_string("System Ready\r\n");
}

void app_task() {
    uint8_t byte;
    frame_t frame;

    if (uart_read_char((char*)&byte)) {
        if (frame_parse_byte(byte, &frame)) {
			// Khi một khung dữ liệu hợp lệ được nhận, xử lý lệnh dựa trên cmd và len


			// Xử lý lệnh dựa trên cmd và len, nếu cmd là CMD_SET_ALL và len là 1, gọi hàm device_service_set_all với giá trị data[0]. Nếu cmd là CMD_SET_SINGLE và len là 2, lấy index từ data[0] và value từ data[1], sau đó gọi device_service_turn_on hoặc device_service_turn_off dựa trên giá trị của value. Sau khi xử lý lệnh, gửi phản hồi "ACK\r\n" nếu lệnh hợp lệ hoặc "NACK\r\n" nếu lệnh không hợp lệ.
            if (frame.cmd == CMD_SET_ALL && frame.len == 1) {
                device_service_set_all(frame.data[0]);
				uart_write_string("ACK\r\n");
            }
            else if (frame.cmd == CMD_SET_SINGLE && frame.len == 2) {
                uint8_t index = frame.data[0];
				uint8_t value = frame.data[1];

                if(value) 
                    device_service_turn_on(index);
                else
					device_service_turn_off(index);

				uart_write_string("ACK\r\n");
            }
			else uart_write_string("NACK\r\n");
        }
    }
}