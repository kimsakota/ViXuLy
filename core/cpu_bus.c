#include "cpu_bus.h"
#include "memory.h"

uint8_t cpu_bus_read(uint16_t addr)
{
    return memory_read(addr);
}

void cpu_bus_write(uint16_t addr, uint8_t data)
{
    memory_write(addr, data);
}
