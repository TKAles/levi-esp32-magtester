#pragma once

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialise NVS flash and open the "magtester" namespace.
 *
 * Must be called once before any other nvs_storage_* function.
 */
esp_err_t nvs_storage_init(void);

/**
 * @brief Save 28 learned ADC counts to NVS.
 *
 * Also sets the has_data flag.
 *
 * @param data  Array of 28 uint8_t values.
 */
esp_err_t nvs_storage_save_learned(const uint8_t data[28]);

/**
 * @brief Load 28 learned ADC counts from NVS.
 *
 * @param data      Output array of 28 uint8_t values.
 * @param has_data  Set to 1 if data was previously stored, else 0.
 */
esp_err_t nvs_storage_load_learned(uint8_t data[28], uint8_t *has_data);

/**
 * @brief Save the hysteresis setting.
 */
esp_err_t nvs_storage_save_hyst(uint8_t hyst);

/**
 * @brief Load the hysteresis setting.  Returns DEFAULT_HYSTERESIS on error.
 */
uint8_t nvs_storage_load_hyst(void);

/**
 * @brief Save the num_samples setting.
 */
esp_err_t nvs_storage_save_nsamples(uint8_t nsamples);

/**
 * @brief Load the num_samples setting.  Returns DEFAULT_NUM_SAMPLES on error.
 */
uint8_t nvs_storage_load_nsamples(void);
