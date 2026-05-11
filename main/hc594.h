#pragma once

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Initialise 74HC594 GPIO lines and clear the shift register.
 *
 * Must be called after spi_bus_init().
 */
esp_err_t hc594_init(void);

/**
 * @brief Assert the chip-select for the given ADC (1–7).
 *
 * Shifts the appropriate active-low byte into the SR, pulses RCLK,
 * then asserts /OE (GPIO low) to enable the outputs.
 *
 * @param adc_num  ADC number 1–7
 */
void hc594_select(uint8_t adc_num);

/**
 * @brief Deassert all chip-selects.
 *
 * Drives /OE high only — the storage register value is unchanged.
 */
void hc594_deselect(void);
