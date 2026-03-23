#ifndef UART_H_
#define UART_H_

#include <stdbool.h>
#include <stdint.h>
/* Build serial communication standard via bit-banging
(time-calculated GPIO toggling instead of hardware UART module) */

/* Configure TX as output, RX as input. Set TX high (Idle state) */
void uart_init();

/* Send 1 byte of data sequentially via GPIO bit-banging. 
Applies delay based on 9600 baud rate (104.15us/bit). */
void uart_write_char(char c);

/* Send a string sequence */
void uart_write_string(const char* s);

// Convert a byte into two hex chars and send via UART
void uart_write_hex_byte(uint8_t byte);

/* Read a single byte in non-blocking mode.
Checks for RX Low (Start bit), reads 8 bits sequentially and delays appropriately. */
bool uart_read_char(char* out);

#endif /* UART_H_ */