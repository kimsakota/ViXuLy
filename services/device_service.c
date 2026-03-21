#include "device_service.h"
#include "../drivers/ppi8255.h"

// Biến toàn cục để lưu trạng thái hiện tại của các thiết bị được điều khiển bởi Port A của 8255. Mỗi bit trong biến này đại diện cho trạng thái của một thiết bị cụ thể (1 = ON, 0 = OFF). Ví dụ, nếu device_state = 0x05 (00000101 trong nhị phân), thì thiết bị 0 và thiết bị 2 đang bật, trong khi các thiết bị khác đang tắt.
static uint8_t device_state = 0;

void device_service_init() {
	device_state = 0;
	ppi8255_write_portA(device_state);
}

void device_service_set_all(uint8_t value) {
	device_state = value;
	ppi8255_write_portA(device_state);
}

void device_service_turn_on(uint8_t index) {
	if (index < 8) {
		device_state |= (1 << index);
		ppi8255_write_portA(device_state);
	}
}

void device_service_turn_off(uint8_t index) {
	if (index < 8) {
		device_state &= ~(1 << index);
		ppi8255_write_portA(device_state);
	}
}

uint8_t device_service_get_state() {
	return device_state;
}