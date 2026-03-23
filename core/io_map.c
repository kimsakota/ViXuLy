#include "io_map.h"
#include "../drivers/ppi8255.h"
#include "../config/system_config.h"

uint8_t io_map_read(uint16_t addr)
{
    switch (addr)
    {
        case 0x8000: return ppi8255_read_portA();
        case 0x8001: return ppi8255_read_portB();
        case 0x8002: return ppi8255_read_portC();
        default:     return 0xFF;
    }
}

void io_map_write(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
        case 0x8000: ppi8255_write_portA(data); break;
        case 0x8001: ppi8255_write_portB(data); break;
        case 0x8002: ppi8255_write_portC(data); break;
        case 0x8003: ppi8255_write(PPI_CONTROL, data); break;
        default: break;
    }
}
