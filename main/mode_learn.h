#pragma once

#include "app_state.h"

/**
 * @brief Enter LEARN mode — show the "Press D1 to Learn" prompt.
 */
void mode_learn_enter(app_state_t *state);

/**
 * @brief Handle D1 press in LEARN mode.
 *
 * Reads NUM_SAMPLES sets of sensor data, averages them, stores to NVS,
 * and updates state->learned / state->has_learned_data.
 */
void mode_learn_action(app_state_t *state);
