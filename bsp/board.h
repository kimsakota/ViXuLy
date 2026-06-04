#ifndef BOARD_H_
#define BOARD_H_

#include <avr/io.h>

// 8255 data bus
#define PPI_DATA_DDR DDRA // Thanh ghi hướng dữ liệu cho cổng A (Data Bus)
#define PPI_DATA_PORT PORTA // Thanh ghi xuất dữ liệu cho cổng A (Data Bus)
#define PPI_DATA_PIN PINA // Thanh ghi đọc dữ liệu từ cổng A (Data Bus)

// Connection between PPI and ATmega (Control Bus)
#define PPI_CTRL_DDR DDRB
#define PPI_CTRL_PORT PORTB

#define PPI_WR_PIN PB0 
#define PPI_RD_PIN PB1
#define PPI_A0_PIN PB2
#define PPI_A1_PIN PB3
#define PPI_CS_PIN PB4 // Kích hoạt 8255 PPI khi CS = 0, không hoạt động khi CS = 1

// ===== ADC0804 + 74HC4051 via PPI 8255 =====
// Port A 8255: điều khiển tải (device)
// Port B 8255: nhận dữ liệu ADC D0..D7
// Port C 8255: xuất tín hiệu điều khiển ADC/mux

#define PPI_ADC_CS_BIT    0  // PC0 -> ADC0804 CS#
#define PPI_ADC_WR_BIT    1  // PC1 -> ADC0804 WR#
#define PPI_ADC_RD_BIT    2  // PC2 -> ADC0804 RD#

#define PPI_ADC_MUX_A_BIT 4  // PC4 -> 74HC4051 A
#define PPI_ADC_MUX_B_BIT 5  // PC5 -> 74HC4051 B
#define PPI_ADC_MUX_C_BIT 6  // PC6 -> 74HC4051 C

// ===== UART SOFT =====
// Theo schematic: UART nằm trên Port D (Port C của ATmega bỏ trống).
#define UART_DDR DDRD
#define UART_PORT PORTD
#define UART_PIN PIND

// Connection between UART and ATmega (TX/RX)
// PD1 is TX (Output), PD0 is RX (Input)
// Virtual Terminal RXD <- PD1 (TX), TXD -> PD0 (RX)
// Lưu ý: PD2 = INTR_0804 (chân ~INTR của ADC0804), không dùng cho UART.
#define UART_TX_PIN PD1
#define UART_RX_PIN PD0

#endif /* BOARD_H_ */