#include "app.h"
#include "../config/system_config.h"
#include "../drivers/ppi8255.h"
#include "../drivers/uart.h"
#include "../services/device_service.h"
#include "../protocol/frame.h"
#include "../protocol/command.h"
#include "../core/memory.h"
#include <util/delay.h>

void app_init() {
    uart_init();
    ppi8255_init();
    memory_init();
    device_service_init();
    frame_init();

    uart_write_string("System Ready\r\n");
}

void app_task() {
    uint8_t byte;
    frame_t frame;

    if (uart_read_char((char*)&byte)) {
        if (frame_parse_byte(byte, &frame)) {
           uart_write_string("FOK\r\n");
			// When a valid data frame is received, process the command based on cmd and len
			// If cmd is CMD_SET_ALL and len is 1, call device_service_set_all with data[0].
            // If cmd is CMD_SET_SINGLE and len is 2, get index from data[0] and value from data[1],
            // then call device_service_turn_on or device_service_turn_off based on value.
            // After processing, send "ACK\r\n" if valid, else "NACK\r\n"
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