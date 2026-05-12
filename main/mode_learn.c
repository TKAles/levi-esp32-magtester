#include "mode_learn.h"
#include "serial_protocol.h"
#include "mag_sensor.h"
#include "nvs_storage.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "mode_learn";

void mode_learn_enter(app_state_t *state)
{
    state->learn_sub = LEARN_SUB_IDLE;
}

void mode_learn_action(app_state_t *state)
{
    state->learn_sub = LEARN_SUB_LEARNING;
    serial_protocol_send(state);

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
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    for (int i = 0; i < NUM_SENSORS; i++) {
        state->learned[i] = (uint8_t)(accumulator[i] / n);
    }

    esp_err_t ret = nvs_storage_save_learned(state->learned);
    if (ret == ESP_OK) {
        state->has_learned_data = 1;
        state->learn_sub = LEARN_SUB_DONE_OK;
        ESP_LOGI(TAG, "Learned data saved to NVS");
    } else {
        state->learn_sub = LEARN_SUB_DONE_ERR;
        ESP_LOGE(TAG, "Failed to save learned data: %s", esp_err_to_name(ret));
    }

    serial_protocol_send(state);

    /* Hold result state visible for 2 s then return to idle */
    vTaskDelay(pdMS_TO_TICKS(2000));
    mode_learn_enter(state);
}
