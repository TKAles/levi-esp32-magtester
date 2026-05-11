#pragma once

#include <stdint.h>

/* =========================================================================
 * Application mode enumeration
 *
 * D0 cycles: TEST → LEARN → VERIFY → OPTIONS → TEST
 * ========================================================================= */
typedef enum {
    APP_MODE_TEST    = 0,
    APP_MODE_LEARN   = 1,
    APP_MODE_VERIFY  = 2,
    APP_MODE_OPTIONS = 3,
    APP_MODE_COUNT   = 4,
} app_mode_t;

/* =========================================================================
 * Shared application state
 * ========================================================================= */
typedef struct {
    app_mode_t  mode;                   /* Current operating mode */

    uint8_t     learned[28];            /* Last learned ADC counts */
    uint8_t     has_learned_data;       /* 1 if NVS contains valid learned data */

    uint8_t     hysteresis;             /* Pass/fail band (ADC counts) */
    uint8_t     num_samples;            /* Averages per verify/learn cycle */

    /* OPTIONS mode cursor (0 = hysteresis, 1 = num_samples) */
    uint8_t     options_cursor;
} app_state_t;
