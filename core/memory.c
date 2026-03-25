#include "memory.h"
#include "io_map.h"
#include "../config/system_config.h"
#include <avr/pgmspace.h>

static const uint8_t rom[ROM_SIZE] PROGMEM = {0};
static uint8_t ram[RAM_SIZE];

void memory_init(void)
{
    for (uint16_t i = 0; i < RAM_SIZE; i++)
        ram[i] = 0;
}

uint8_t memory_read(uint16_t addr)
{
    if (addr >= MEM_ROM_START && addr <= MEM_ROM_END)
        return pgm_read_byte(&rom[addr - MEM_ROM_START]);
    else if (addr >= MEM_RAM_START && addr <= MEM_RAM_END)
        return ram[addr - MEM_RAM_START];
    else if (addr >= MEM_8255_START && addr <= MEM_8255_END)
        return io_map_read(addr);

    return 0xFF;
}

void memory_write(uint16_t addr, uint8_t data)
{
    if (addr >= MEM_RAM_START && addr <= MEM_RAM_END)
        ram[addr - MEM_RAM_START] = data;
    else if (addr >= MEM_8255_START && addr <= MEM_8255_END)
        io_map_write(addr, data);
}
