#ifndef ADC0804_H_
#define ADC0804_H_

#include <stdint.h>

// Initialize ADC path over 8255:
// - Port B used to read ADC0804 8-bit data.
// - Port C used to drive ADC0804 control and 74HC4051 channel select.
void adc0804_init(void);

// Select analog channel (0–7) via 74HC4051 (through 8255 Port C),
// start conversion and read the 8-bit ADC0804 value through 8255 Port B.
uint8_t adc0804_read(uint8_t channel);

#endif /* ADC0804_H_ */