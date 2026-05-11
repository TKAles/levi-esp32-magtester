#include "mag_sensor.h"
#include "adc084s021.h"
#include "config.h"

void mag_sensor_read_all(uint8_t results[28])
{
    for (uint8_t adc = 1; adc <= NUM_ADCS; adc++) {
        for (uint8_t ch = 0; ch < CHANNELS_PER_ADC; ch++) {
            uint8_t idx = (adc - 1) * CHANNELS_PER_ADC + ch;
            results[idx] = adc084s021_read_channel(adc, ch);
        }
    }
}
