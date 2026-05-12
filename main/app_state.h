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

/* LEARN mode sub-states */
typedef enum {
    LEARN_SUB_IDLE     = 0,
    LEARN_SUB_LEARNING = 1,
    LEARN_SUB_DONE_OK  = 2,
    LEARN_SUB_DONE_ERR = 3,
} learn_sub_t;

/* VERIFY mode sub-states */
typedef enum {
    VERIFY_SUB_IDLE      = 0,
    VERIFY_SUB_MEASURING = 1,
    VERIFY_SUB_PASS      = 2,
    VERIFY_SUB_FAIL      = 3,
} verify_sub_t;

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

    /* Live sensor readings, updated by mode_test_update */
    uint8_t     sensors[28];

    /* LEARN mode sub-state */
    uint8_t     learn_sub;              /* learn_sub_t */

    /* VERIFY mode sub-state and results */
    uint8_t     verify_sub;             /* verify_sub_t */
    uint8_t     verify_bad[28];         /* indices of sensors that failed */
    uint8_t     verify_num_bad;         /* count of failed sensors */
    uint8_t     verify_averaged[28];    /* averaged readings from last verify run */
} app_state_t;
