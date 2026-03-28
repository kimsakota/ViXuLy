#include "measurement_service.h"
#include "../drivers/adc0804.h"
#include "../config/system_config.h"

// 100 samples x ~217us/sample = ~21.7ms > 20ms → guaranteed >= 1 full 50Hz cycle.
// Peak tracking is phase-independent: stable regardless of sampling window alignment.
#define PEAK_SAMPLE_COUNT 100
// ACS712 outputs VCC/2 at zero current → ADC midpoint = 128
#define ADC_ZERO_OFFSET   128

void measurement_service_init(void)
{
    adc0804_init();
}

// Returns I_peak in LSB units (0..128).
// Convert on receiver: I_peak_A = peak_lsb * VCC / 256 / sensitivity_V_per_A
// I_rms_A  = I_peak_A / sqrt(2)
// P_W      = V_rms * I_rms_A  (resistive load)
uint8_t measurement_service_read(uint8_t index)
{
    if (index >= DEVICE_COUNT)
        return 0;

    uint8_t peak = 0;
    for (uint8_t i = 0; i < PEAK_SAMPLE_COUNT; i++) {
        uint8_t raw = adc0804_read(index);
        int16_t diff = (int16_t)raw - ADC_ZERO_OFFSET;
        uint8_t abs_diff = (diff < 0) ? (uint8_t)(-diff) : (uint8_t)diff;
        if (abs_diff > peak)
            peak = abs_diff;
    }
    return peak;
}
