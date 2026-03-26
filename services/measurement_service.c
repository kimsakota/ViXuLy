#include "measurement_service.h"
#include "../drivers/adc0804.h"
#include "../config/system_config.h"

void measurement_service_init(void)
{
    adc0804_init();
}

uint8_t measurement_service_read(uint8_t index)
{
    if (index >= DEVICE_COUNT)
        return 0;
    return adc0804_read(index);
}
