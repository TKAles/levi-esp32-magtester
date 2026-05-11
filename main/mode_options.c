#include "mode_options.h"
#include "display.h"
#include "nvs_storage.h"
#include "config.h"

#include "esp_log.h"

#include <stdio.h>

static const char *TAG = "mode_options";

/* Number of adjustable settings */
#define NUM_SETTINGS 2

/* Setting indices */
#define SETTING_HYST     0
#define SETTING_NSAMPLES 1

/* Y positions for each settings row */
static const int SETTING_Y[NUM_SETTINGS] = { 40, 65 };

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

static void draw_screen(app_state_t *state)
{
    display_clear(COLOR_BLACK);
    display_draw_string(4, 4, "OPTIONS", COLOR_MAGENTA, COLOR_BLACK, 2);
    display_draw_string(4, 22, "D1: change  D0: next/exit", COLOR_CYAN, COLOR_BLACK, 1);

    /* HYSTERESIS row */
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "HYSTERESIS : %2u", state->hysteresis);
        uint16_t fg = (state->options_cursor == SETTING_HYST) ? COLOR_YELLOW : COLOR_WHITE;
        uint16_t bg = (state->options_cursor == SETTING_HYST) ? COLOR_DARK_GREEN : COLOR_BLACK;
        if (state->options_cursor == SETTING_HYST) {
            display_fill_rect(0, SETTING_Y[SETTING_HYST] - 1,
                              TFT_WIDTH, 11, COLOR_DARK_GREEN);
            display_draw_string(2, SETTING_Y[SETTING_HYST], ">", COLOR_YELLOW, bg, 1);
        }
        display_draw_string(12, SETTING_Y[SETTING_HYST], buf, fg, bg, 1);
    }

    /* NUM SAMPLES row */
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "NUM SAMPLES: %2u", state->num_samples);
        uint16_t fg = (state->options_cursor == SETTING_NSAMPLES) ? COLOR_YELLOW : COLOR_WHITE;
        uint16_t bg = (state->options_cursor == SETTING_NSAMPLES) ? COLOR_DARK_GREEN : COLOR_BLACK;
        if (state->options_cursor == SETTING_NSAMPLES) {
            display_fill_rect(0, SETTING_Y[SETTING_NSAMPLES] - 1,
                              TFT_WIDTH, 11, COLOR_DARK_GREEN);
            display_draw_string(2, SETTING_Y[SETTING_NSAMPLES], ">", COLOR_YELLOW, bg, 1);
        }
        display_draw_string(12, SETTING_Y[SETTING_NSAMPLES], buf, fg, bg, 1);
    }

    display_draw_string(4, 100, "D0 past last: exit", COLOR_CYAN, COLOR_BLACK, 1);
    display_flush();
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void mode_options_enter(app_state_t *state)
{
    state->options_cursor = 0;
    draw_screen(state);
}

int mode_options_d0(app_state_t *state)
{
    state->options_cursor++;
    if (state->options_cursor >= NUM_SETTINGS) {
        /* Past last setting -> signal exit to TEST */
        state->options_cursor = 0;
        return 1;
    }
    draw_screen(state);
    return 0;
}

void mode_options_d1(app_state_t *state)
{
    if (state->options_cursor == SETTING_HYST) {
        state->hysteresis++;
        if (state->hysteresis > MAX_HYSTERESIS) {
            state->hysteresis = MIN_HYSTERESIS;
        }
        esp_err_t ret = nvs_storage_save_hyst(state->hysteresis);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save hysteresis: %s", esp_err_to_name(ret));
        }
    } else if (state->options_cursor == SETTING_NSAMPLES) {
        state->num_samples++;
        if (state->num_samples > MAX_NUM_SAMPLES) {
            state->num_samples = MIN_NUM_SAMPLES;
        }
        esp_err_t ret = nvs_storage_save_nsamples(state->num_samples);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save num_samples: %s", esp_err_to_name(ret));
        }
    }
    draw_screen(state);
}
