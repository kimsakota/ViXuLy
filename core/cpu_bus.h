#ifndef CPU_BUS_H
#define CPU_BUS_H

#include <stdint.h>

uint8_t cpu_bus_read(uint16_t addr);
void cpu_bus_write(uint16_t addr, uint8_t data);

#endif
