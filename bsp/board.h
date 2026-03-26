#ifndef BOARD_H_
#define BOARD_H_

#include <avr/io.h>

// 8255 data bus
#define PPI_DATA_DDR DDRD // Thanh ghi hướng dữ liệu cho cổng D (Data Bus)
#define PPI_DATA_PORT PORTD // Thanh ghi xuất dữ liệu cho cổng D (Data Bus)
#define PPI_DATA_PIN PIND // Thanh ghi đọc dữ liệu từ cổng D (Data Bus)

// Connection between PPI and ATmega (Control Bus)
#define PPI_CTRL_DDR DDRB
#define PPI_CTRL_PORT PORTB

#define PPI_WR_PIN PB0 
#define PPI_RD_PIN PB1
#define PPI_A0_PIN PB2
#define PPI_A1_PIN PB3
#define PPI_CS_PIN PB4 // Kích hoạt 8255 PPI khi CS = 0, không hoạt động khi CS = 1

// ===== ADC0804 =====
// ADC0804 shares the data bus (PORTD) with PPI 8255
// Control signals and analog mux (CD4051) channel select are on PORTA
#define ADC_DDR      DDRA
#define ADC_PORT     PORTA
#define ADC_PIN      PINA

#define ADC_CS_PIN   PA0  // Chip Select  (active low)
#define ADC_WR_PIN   PA1  // Start Conv.  (active low, rising edge triggers conversion)
#define ADC_RD_PIN   PA2  // Read output  (active low)
#define ADC_INTR_PIN PA3  // Conv. done   (input, active low from ADC0804)

// 74HC4051 analog mux channel select (PA4–PA6 → A, B, C → selects 1 of 8 channels)
#define ADC_CH_A_PIN PA4
#define ADC_CH_B_PIN PA5
#define ADC_CH_C_PIN PA6

// ===== UART SOFT =====
#define UART_DDR DDRC 
#define UART_PORT PORTC 
#define UART_PIN PINC

// Connection between UART and ATmega (TX/RX)
// PC0 is TX (Output), PC1 is RX (Input)
// Virtual Terminal TXD -> PC1, RXD -> PC0
#define UART_TX_PIN PC0
#define UART_RX_PIN PC1

#endif /* BOARD_H_ */