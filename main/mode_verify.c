#include "mode_verify.h"
#include "serial_protocol.h"
#include "mag_sensor.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "mode_verify";

void mode_verify_enter(app_state_t *state)
{
    state->verify_sub     = VERIFY_SUB_IDLE;
    state->verify_num_bad = 0;
}

void mode_verify_action(app_state_t *state)
{
    if (!state->has_learned_data) {
        /* Nothing to compare against — stay idle, companion app shows the error */
        return;
    }

    state->verify_sub = VERIFY_SUB_MEASURING;
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
        state->verify_averaged[i] = (uint8_t)(accumulator[i] / n);
    }

    state->verify_num_bad = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        int dev = (int)state->verify_averaged[i] - (int)state->learned[i];
        if (dev < 0) dev = -dev;
        if ((uint8_t)dev > state->hysteresis) {
            state->verify_bad[state->verify_num_bad++] = (uint8_t)i;
        }
    }

    state->verify_sub = (state->verify_num_bad == 0) ? VERIFY_SUB_PASS : VERIFY_SUB_FAIL;

    ESP_LOGI(TAG, "Verify: %u/%u sensors failed", state->verify_num_bad, NUM_SENSORS);

    serial_protocol_send(state);

    /* Hold result visible for 3 s then return to idle */
    vTaskDelay(pdMS_TO_TICKS(3000));
    mode_verify_enter(state);
}
