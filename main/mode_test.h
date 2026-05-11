#pragma once

#include "app_state.h"

/**
 * @brief Enter TEST mode — clears display and draws static header.
 */
void mode_test_enter(app_state_t *state);

/**
 * @brief Update TEST mode — reads all sensors and redraws the 7x4 grid.
 *
 * Should be called approximately every TEST_UPDATE_MS milliseconds.
 */
void mode_test_update(app_state_t *state);
