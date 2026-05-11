#include "mode_learn.h"
#include "display.h"
#include "mag_sensor.h"
#include "nvs_storage.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>

static const char *TAG = "mode_learn";

void mode_learn_enter(app_state_t *state)
{
    (void)state;
    display_clear(COLOR_BLACK);
    display_draw_string(4, 4, "LEARN MODE", COLOR_YELLOW, COLOR_BLACK, 2);
    display_draw_string(4, 30, "Press D1 to Learn", COLOR_WHITE, COLOR_BLACK, 1);

    if (state->has_learned_data) {
        display_draw_string(4, 50, "Prev data stored", COLOR_DARK_GREEN, COLOR_BLACK, 1);
    } else {
        display_draw_string(4, 50, "No data stored", COLOR_DARK_RED, COLOR_BLACK, 1);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "Samples: %u", state->num_samples);
    display_draw_string(4, 65, buf, COLOR_CYAN, COLOR_BLACK, 1);

    display_flush();
}

void mode_learn_action(app_state_t *state)
{
    /* Show "Learning..." */
    display_clear(COLOR_BLACK);
    display_draw_string(4, 4, "LEARN MODE", COLOR_YELLOW, COLOR_BLACK, 2);
    display_draw_string(4, 30, "Learning...", COLOR_WHITE, COLOR_BLACK, 2);
    display_flush();

    uint32_t accumulator[NUM_SENSORS];
    memset(accumulator, 0, sizeof(accumulator));

    uint8_t readings[NUM_SENSORS];
    uint8_t n = state->num_samples;
    if (n < MIN_NUM_SAMPLES) n = MIN_NUM_SAMPLES;
    if (n > MAX_NUM_SAMPLES) n = MAX_NUM_SAMPLES;

    for (uint8_t s = 0; s < n; s++) {
        mag_sensor_read_all(readings);
        for (int i = 0; i < NUM_SENSORS; i++) {
            accumulator[i] += readings[i];
        }
        /* Brief yield between samples */
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    /* Average */
    for (int i = 0; i < NUM_SENSORS; i++) {
        state->learned[i] = (uint8_t)(accumulator[i] / n);
    }

    /* Persist to NVS */
    esp_err_t ret = nvs_storage_save_learned(state->learned);
    if (ret == ESP_OK) {
        state->has_learned_data = 1;
        ESP_LOGI(TAG, "Learned data saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save learned data: %s", esp_err_to_name(ret));
    }

    /* Show "Done!" */
    display_clear(COLOR_BLACK);
    display_draw_string(4, 4, "LEARN MODE", COLOR_YELLOW, COLOR_BLACK, 2);
    if (ret == ESP_OK) {
        display_draw_string(4, 40, "Done!", COLOR_GREEN, COLOR_BLACK, 3);
        display_draw_string(4, 90, "28 sensors averaged", COLOR_WHITE, COLOR_BLACK, 1);
    } else {
        display_draw_string(4, 40, "NVS Error!", COLOR_RED, COLOR_BLACK, 2);
    }
    display_flush();

    /* Hold result on screen for 2 s, then re-show prompt */
    vTaskDelay(pdMS_TO_TICKS(2000));
    mode_learn_enter(state);
}
