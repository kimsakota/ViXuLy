#include "ppi8255.h"
#include "../bsp/board.h"
#include "../config/system_config.h" 
#include <util/delay.h>

static void set_addr(uint8_t addr) {
    if (addr & 0x01)
        PPI_CTRL_PORT |= (1 << PPI_A0_PIN);
    else
        PPI_CTRL_PORT &= ~(1 << PPI_A0_PIN);

    if (addr & 0x02)
        PPI_CTRL_PORT |= (1 << PPI_A1_PIN);
    else
        PPI_CTRL_PORT &= ~(1 << PPI_A1_PIN);
}

void ppi8255_write(ppi_reg_t reg, uint8_t data) {
    PPI_DATA_DDR = 0xFF; // Set data bus to output
	PPI_DATA_PORT = data; // Put data on bus

    set_addr(reg);

    PPI_CTRL_PORT |= (1 << PPI_RD_PIN);
    PPI_CTRL_PORT &= ~(1 << PPI_CS_PIN);

    PPI_CTRL_PORT &= ~(1 << PPI_WR_PIN);

	// Chờ 1 micro giây, đây là yêu cầu timing từ datasheet của 8255 để đảm bảo dữ liệu được ghi đúng cách. Tùy thuộc vào tốc độ CPU và yêu cầu cụ thể, bạn có thể cần điều chỉnh thời gian này.
    _delay_us(1); 

    PPI_CTRL_PORT |= (1 << PPI_WR_PIN);

    PPI_CTRL_PORT |= (1 << PPI_CS_PIN);
}

uint8_t ppi8255_read(ppi_reg_t reg) {
    uint8_t data;
    PPI_DATA_DDR = 0x00; // Set data bus to input
    PPI_DATA_PORT = 0x00; // Disable pull-ups

    set_addr(reg);

    PPI_CTRL_PORT |= (1 << PPI_WR_PIN);
    PPI_CTRL_PORT &= ~(1 << PPI_CS_PIN);

    PPI_CTRL_PORT &= ~(1 << PPI_RD_PIN);
    _delay_us(1); 
    data = PPI_DATA_PIN;

    PPI_CTRL_PORT |= (1 << PPI_RD_PIN);

    PPI_CTRL_PORT |= (1 << PPI_CS_PIN);
    return data;
}

void ppi8255_init(void) {
    // set control pins output
    PPI_CTRL_DDR |= (1 << PPI_WR_PIN) | (1 << PPI_RD_PIN) | (1 << PPI_A0_PIN) |
                    (1 << PPI_A1_PIN) | (1 << PPI_CS_PIN);

    // set data bus output
    PPI_DATA_DDR = 0xFF;

    // default state
    PPI_CTRL_PORT |= (1 << PPI_WR_PIN);
    PPI_CTRL_PORT |= (1 << PPI_RD_PIN);
    PPI_CTRL_PORT |= (1 << PPI_CS_PIN);

    ppi8255_write(PPI_CONTROL, 0x80); // mode 0, all output
}

void ppi8255_write_portA(uint8_t value) { ppi8255_write(PPI_PORT_A, value); }
void ppi8255_write_portB(uint8_t value) { ppi8255_write(PPI_PORT_B, value); }
void ppi8255_write_portC(uint8_t value) { ppi8255_write(PPI_PORT_C, value); }

uint8_t ppi8255_read_portA(void) { return ppi8255_read(PPI_PORT_A); }
uint8_t ppi8255_read_portB(void) { return ppi8255_read(PPI_PORT_B); }
uint8_t ppi8255_read_portC(void) { return ppi8255_read(PPI_PORT_C); }