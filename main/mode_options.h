#pragma once

#include "app_state.h"

/**
 * @brief Enter OPTIONS mode — display settings with cursor on first item.
 */
void mode_options_enter(app_state_t *state);

/**
 * @brief Handle D0 press in OPTIONS mode.
 *
 * Advances the cursor.  When the cursor passes the last setting the function
 * returns 1 to signal that the caller should transition to TEST mode.
 *
 * @return 1 if caller should exit to TEST mode, 0 otherwise.
 */
int mode_options_d0(app_state_t *state);

/**
 * @brief Handle D1 press in OPTIONS mode.
 *
 * Increments the currently selected setting and redraws the screen.
 */
void mode_options_d1(app_state_t *state);
