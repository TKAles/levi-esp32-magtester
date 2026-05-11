#pragma once

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Read one channel from the specified ADC084S021.
 *
 * Asserts the ADC chip-select via the 74HC594, performs a 16-bit SPI
 * conversion, then deasserts CS.
 *
 * @param adc_num  ADC number 1–7
 * @param channel  Channel 0–3
 * @return         8-bit ADC result (bits [11:4] of the SPI reply)
 */
uint8_t adc084s021_read_channel(uint8_t adc_num, uint8_t channel);
