#ifndef ADC0804_H_
#define ADC0804_H_

#include <stdint.h>

// Initialize ADC0804 control pins (CS, WR, RD as output; INTR as input)
// and 74HC4051 channel-select pins (CH_A, CH_B, CH_C).
void adc0804_init(void);

// Select analog channel (0–7) via 74HC4051, start a conversion on ADC0804,
// wait for INTR then read and return the 8-bit result.
// Shares PORTD data bus with PPI 8255; bus direction is restored after read.
uint8_t adc0804_read(uint8_t channel);

#endif /* ADC0804_H_ */