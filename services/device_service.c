#include "device_service.h"
#include "../drivers/ppi8255.h"

static uint8_t device_state = 0;

void device_service_init(void)
{
	device_state = 0x00;
	ppi8255_write_portA(device_state);
}

void device_service_set_all(uint8_t value)
{
	device_state = value;
	ppi8255_write_portA(device_state);
}

void device_service_turn_on(uint8_t index)
{
	if (index < 8)
	{
		device_state |= (1 << index);
		ppi8255_write_portA(device_state);
	}
}

void device_service_turn_off(uint8_t index)
{
	if (index < 8)
	{
		device_state &= ~(1 << index);
		ppi8255_write_portA(device_state);
	}
}

void device_service_toggle(uint8_t index)
{
	if (index < 8)
	{
		device_state ^= (1 << index);
		ppi8255_write_portA(device_state);
	}
}

uint8_t device_service_get_state(void)
{
	return device_state;
}
