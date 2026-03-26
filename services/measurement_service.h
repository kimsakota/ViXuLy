#ifndef MEASUREMENT_SERVICE_H
#define MEASUREMENT_SERVICE_H

#include <stdint.h>

// Initialize measurement service (calls adc0804_init internally).
void measurement_service_init(void);

// Read the 8-bit ADC value for device at given index (0-7).
// Channel index corresponds to the same bit position as 8255 Port A.
uint8_t measurement_service_read(uint8_t index);

#endif /* MEASUREMENT_SERVICE_H */
