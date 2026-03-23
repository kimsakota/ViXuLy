#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

void memory_init(void);
uint8_t memory_read(uint16_t addr);
void memory_write(uint16_t addr, uint8_t data);

#endif
