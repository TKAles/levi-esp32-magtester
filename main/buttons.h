#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* =========================================================================
 * Button event types
 * ========================================================================= */
typedef enum {
    PRESS_TYPE_SHORT = 0,
} press_type_t;

typedef struct {
    uint8_t      button_num;  /* 0 = D0, 1 = D1 */
    press_type_t press_type;
} button_event_t;

/* =========================================================================
 * API
 * ========================================================================= */

/**
 * @brief Initialise button GPIOs, install GPIO ISR service, and create the
 *        event queue.
 *
 * @return ESP_OK on success.
 */
esp_err_t buttons_init(void);

/**
 * @brief Return the FreeRTOS queue that receives button_event_t items.
 *
 * The caller should use xQueueReceive() on this queue.
 */
QueueHandle_t buttons_get_queue(void);
