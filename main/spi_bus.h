#pragma once

#include "driver/spi_master.h"

/**
 * @brief Initialise SPI2 bus and create the shared ADC device handle.
 *
 * The TFT is managed separately by esp_lcd; this function only sets up the
 * bus and the ADC/shift-register device (cs_io_num = -1, software CS).
 */
esp_err_t spi_bus_init(void);

/**
 * @brief Return the SPI device handle used for ADC and shift-register access.
 */
spi_device_handle_t spi_get_adc_handle(void);
