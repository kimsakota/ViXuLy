#include "adc0804.h"
#include "../bsp/board.h"
#include "ppi8255.h"
#include <util/delay.h>

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
    // Select channel on 74HC4051 by Port C bits (A=bit0, B=bit1, C=bit2)
    set_portc_bit(PPI_ADC_MUX_A_BIT, (channel & 0x01) != 0);
    set_portc_bit(PPI_ADC_MUX_B_BIT, (channel & 0x02) != 0);
    set_portc_bit(PPI_ADC_MUX_C_BIT, (channel & 0x04) != 0);

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
