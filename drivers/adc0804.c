#include "adc0804.h"
#include "../bsp/board.h"
#include <util/delay.h>

void adc0804_init(void)
{
    // CS, WR, RD, CH_A/B/C as outputs; INTR as input
    ADC_DDR |=  (1 << ADC_CS_PIN) | (1 << ADC_WR_PIN) | (1 << ADC_RD_PIN)
              | (1 << ADC_CH_A_PIN) | (1 << ADC_CH_B_PIN) | (1 << ADC_CH_C_PIN);
    ADC_DDR &= ~(1 << ADC_INTR_PIN);

    // Default inactive state: CS=HIGH, WR=HIGH, RD=HIGH
    ADC_PORT |=  (1 << ADC_CS_PIN) | (1 << ADC_WR_PIN) | (1 << ADC_RD_PIN);
    // Channel select default: channel 0
    ADC_PORT &= ~((1 << ADC_CH_A_PIN) | (1 << ADC_CH_B_PIN) | (1 << ADC_CH_C_PIN));
}

uint8_t adc0804_read(uint8_t channel)
{
    // Select one of 8 analog channels via 74HC4051 (A=bit0, B=bit1, C=bit2)
    if (channel & 0x01) ADC_PORT |=  (1 << ADC_CH_A_PIN);
    else                ADC_PORT &= ~(1 << ADC_CH_A_PIN);

    if (channel & 0x02) ADC_PORT |=  (1 << ADC_CH_B_PIN);
    else                ADC_PORT &= ~(1 << ADC_CH_B_PIN);

    if (channel & 0x04) ADC_PORT |=  (1 << ADC_CH_C_PIN);
    else                ADC_PORT &= ~(1 << ADC_CH_C_PIN);

    // Start conversion: CS low, WR low then high (rising edge triggers ADC0804)
    ADC_PORT &= ~(1 << ADC_CS_PIN);
    ADC_PORT &= ~(1 << ADC_WR_PIN);
    _delay_us(1);
    ADC_PORT |=  (1 << ADC_WR_PIN);

    // Wait for INTR to go LOW (conversion complete, typ ~110 us)
    uint8_t timeout = 200;
    while ((ADC_PIN & (1 << ADC_INTR_PIN)) && timeout--)
        _delay_us(1);

    // Read result: switch shared data bus to input, assert RD low
    PPI_DATA_DDR  = 0x00;
    PPI_DATA_PORT = 0x00;

    ADC_PORT &= ~(1 << ADC_RD_PIN);
    _delay_us(1);
    uint8_t data = PPI_DATA_PIN;
    ADC_PORT |=  (1 << ADC_RD_PIN);
    ADC_PORT |=  (1 << ADC_CS_PIN);

    // Restore data bus to output for PPI 8255
    PPI_DATA_DDR = 0xFF;

    return data;
}
