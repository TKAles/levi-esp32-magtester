#include "mode_options.h"
#include "nvs_storage.h"
#include "config.h"

#include "esp_log.h"

#define NUM_SETTINGS     2
#define SETTING_HYST     0
#define SETTING_NSAMPLES 1

static const char *TAG = "mode_options";

void mode_options_enter(app_state_t *state)
{
    state->options_cursor = 0;
}

int mode_options_d0(app_state_t *state)
{
    state->options_cursor++;
    if (state->options_cursor >= NUM_SETTINGS) {
        state->options_cursor = 0;
        return 1;  /* signal exit to TEST */
    }
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
}
