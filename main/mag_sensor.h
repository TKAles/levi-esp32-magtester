#pragma once

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Read all 28 Hall sensors into @p results.
 *
 * Sensor index mapping:
 *   index = (adc_num - 1) * 4 + channel
 *
 * @param results  Output array of 28 uint8_t ADC counts.
 */
void mag_sensor_read_all(uint8_t results[28]);
