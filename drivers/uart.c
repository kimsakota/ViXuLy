#include "uart.h"
#include "../bsp/board.h"
#include "../config/system_config.h" 
#include <util/delay.h>

void uart_init()
{
	// Cấu hình chân TX là output, RX là input
	UART_DDR |= (1 << UART_TX_PIN);
	UART_DDR &= ~(1 << UART_RX_PIN);

	// Đặt trạng thái TX và RX lên mức High (Idle state)
	UART_PORT |= (1 << UART_TX_PIN);
	UART_PORT |= (1 << UART_RX_PIN);
}

void uart_write_char(char c)
{
	// start bit
	UART_PORT &= ~(1 << UART_TX_PIN);
	_delay_us(104);
	
	for (uint8_t i = 0; i < 8; i++)
	{
		if (c & 0x01)
			UART_PORT |= (1 << UART_TX_PIN);
		else
			UART_PORT &= ~(1 << UART_TX_PIN);

		_delay_us(104);
		c >>= 1;
	}

	// stop bit
	UART_PORT |= (1 << UART_TX_PIN);
	_delay_us(104);
}

void uart_write_string(const char* s)
{
	while (*s)
		uart_write_char(*s++);
}

bool uart_read_char(char* out)
{
	// Check for start bit (RX goes Low)
	if (UART_PIN & (1 << UART_RX_PIN))
		return false;

	// Wait 52us to sample in the middle of the start bit
	_delay_us(52);

	// If RX is High, it was a noise glitch, not a valid start bit
	if (UART_PIN & (1 << UART_RX_PIN))
		return false;

	_delay_us(104);

	uint8_t data = 0;

	for (uint8_t i = 0; i < 8; i++)
	{
		data >>= 1;
		if (UART_PIN & (1 << UART_RX_PIN))
			data |= 0x80;

		_delay_us(104);
	}
	
	// Wait for stop bit
	_delay_us(104);

	// Trả về dữ liệu đã đọc qua con trỏ out
	*out = data;
	return true;
}