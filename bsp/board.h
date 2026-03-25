#ifndef BOARD_H_
#define BOARD_H_

#include <avr/io.h>

// 8255 data bus
#define PPI_DATA_DDR DDRD
#define PPI_DATA_PORT PORTD
#define PPI_DATA_PIN PIND

// Connection between PPI and ATmega (Control Bus)
#define PPI_CTRL_DDR DDRB
#define PPI_CTRL_PORT PORTB

#define PPI_WR_PIN PB0
#define PPI_RD_PIN PB1
#define PPI_A0_PIN PB2
#define PPI_A1_PIN PB3
#define PPI_CS_PIN PB4

// ===== UART SOFT =====
#define UART_PORT PORTC
#define UART_DDR DDRC
#define UART_PIN PINC

// Connection between UART and ATmega (TX/RX)
// PC0 is TX (Output), PC1 is RX (Input)
// Virtual Terminal TXD -> PC1, RXD -> PC0
#define UART_TX_PIN PC0
#define UART_RX_PIN PC1

#endif /* BOARD_H_ */