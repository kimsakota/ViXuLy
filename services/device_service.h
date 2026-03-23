#ifndef DEVICE_SERVICE_H
#define DEVICE_SERVICE_H

#include <stdint.h>

void device_service_init(void);
void device_service_set_all(uint8_t value);
void device_service_turn_on(uint8_t index);
void device_service_turn_off(uint8_t index);
void device_service_toggle(uint8_t index);
uint8_t device_service_get_state(void);

#endif /* DEVICE_SERVICE_H_ */