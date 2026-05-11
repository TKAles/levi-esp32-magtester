#pragma once

#include "app_state.h"

/**
 * @brief Enter VERIFY mode — show the "Press D1 to Verify" prompt.
 */
void mode_verify_enter(app_state_t *state);

/**
 * @brief Handle D1 press in VERIFY mode.
 *
 * Reads NUM_SAMPLES sets of sensor data, averages them, compares to
 * stored learned values within hysteresis, and displays PASS or FAIL.
 */
void mode_verify_action(app_state_t *state);
