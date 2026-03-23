#ifndef IO_MAP_H
#define IO_MAP_H

#include <stdint.h>

uint8_t io_map_read(uint16_t addr);
void io_map_write(uint16_t addr, uint8_t data);

#endif
