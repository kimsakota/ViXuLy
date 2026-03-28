#include "adc0804.h"
#include "../bsp/board.h"
#include "ppi8255.h"
#include <util/delay.h>

// Biến này để lưu trạng thái hiện tại của Port C, giúp chúng ta dễ dàng cập nhật từng bit mà không ảnh hưởng đến các bit khác.
static uint8_t portc_shadow;

static void set_portc_bit(uint8_t bit, uint8_t high)
{
    if (high)
        portc_shadow |= (1 << bit);
    else
        portc_shadow &= ~(1 << bit);
    ppi8255_write_portC(portc_shadow);
}

void adc0804_init(void)
{
    portc_shadow = (1 << PPI_ADC_CS_BIT) | (1 << PPI_ADC_WR_BIT) | (1 << PPI_ADC_RD_BIT);
    ppi8255_write_portC(portc_shadow);
}

uint8_t adc0804_read(uint8_t channel)
{
    // Set all 3 mux bits in one write to avoid glitching through wrong channels.
    // PPI_ADC_MUX_A_BIT=4, B=5, C=6 are consecutive → channel bits map directly.
    uint8_t mux_mask = (1 << PPI_ADC_MUX_A_BIT) | (1 << PPI_ADC_MUX_B_BIT) | (1 << PPI_ADC_MUX_C_BIT);
    portc_shadow = (portc_shadow & ~mux_mask) | (uint8_t)((channel & 0x07) << PPI_ADC_MUX_A_BIT);
    ppi8255_write_portC(portc_shadow);
    _delay_us(10); // Wait for mux output and ADC input capacitor to settle

    // Start conversion: CS low, WR low then high
    set_portc_bit(PPI_ADC_CS_BIT, 0);
    set_portc_bit(PPI_ADC_WR_BIT, 0);
    _delay_us(1);
    set_portc_bit(PPI_ADC_WR_BIT, 1);

    // Conversion time for ADC0804 (polling via fixed delay)
    _delay_us(120);

    // Read converted data via Port B of 8255
    set_portc_bit(PPI_ADC_RD_BIT, 0);
    _delay_us(1);
    uint8_t data = ppi8255_read_portB();
    set_portc_bit(PPI_ADC_RD_BIT, 1);
    set_portc_bit(PPI_ADC_CS_BIT, 1);

    return data;
}
