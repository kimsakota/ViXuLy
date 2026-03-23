#ifndef PPI8255_H_
#define PPI8255_H_

#include <stdint.h>

typedef enum
{
    PPI_PORT_A = 0,
    PPI_PORT_B = 1,
    PPI_PORT_C = 2,
    PPI_CONTROL = 3
} ppi_reg_t;

void ppi8255_init(void);
void ppi8255_write(ppi_reg_t reg, uint8_t value);
uint8_t ppi8255_read(ppi_reg_t reg);

void ppi8255_write_portA(uint8_t value);
void ppi8255_write_portB(uint8_t value);
void ppi8255_write_portC(uint8_t value);

uint8_t ppi8255_read_portA(void);
uint8_t ppi8255_read_portB(void);
uint8_t ppi8255_read_portC(void);

#endif /* PPI8255_H_ */