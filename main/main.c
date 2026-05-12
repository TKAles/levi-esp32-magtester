#include "config.h"
#include "spi_bus.h"
#include "hc594.h"
#include "adc084s021.h"
#include "mag_sensor.h"
#include "serial_protocol.h"
#include "nvs_storage.h"
#include "buttons.h"
#include "app_state.h"
#include "mode_test.h"
#include "mode_learn.h"
#include "mode_verify.h"
#include "mode_options.h"

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "main";

/* =========================================================================
 * Initialisation
 * ========================================================================= */

static void hw_init(void)
{
    ESP_ERROR_CHECK(spi_bus_init());
    ESP_ERROR_CHECK(hc594_init());
    ESP_ERROR_CHECK(nvs_storage_init());
    ESP_ERROR_CHECK(buttons_init());
    serial_protocol_init();
}

static void app_state_init(app_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->mode           = APP_MODE_TEST;
    state->options_cursor = 0;
    state->learn_sub      = LEARN_SUB_IDLE;
    state->verify_sub     = VERIFY_SUB_IDLE;

    state->hysteresis  = nvs_storage_load_hyst();
    state->num_samples = nvs_storage_load_nsamples();
    nvs_storage_load_learned(state->learned, &state->has_learned_data);
}

/* =========================================================================
 * Mode transition helper
 * ========================================================================= */

static void enter_mode(app_state_t *state, app_mode_t new_mode)
{
    state->mode = new_mode;
    switch (new_mode) {
        case APP_MODE_TEST:    mode_test_enter(state);    break;
        case APP_MODE_LEARN:   mode_learn_enter(state);   break;
        case APP_MODE_VERIFY:  mode_verify_enter(state);  break;
        case APP_MODE_OPTIONS: mode_options_enter(state); break;
        default: break;
    }
}

static void cycle_mode(app_state_t *state)
{
    app_mode_t next = (app_mode_t)((state->mode + 1) % APP_MODE_COUNT);
    enter_mode(state, next);
}

/* =========================================================================
 * Main entry point
 * ========================================================================= */

void app_main(void)
{
    ESP_LOGI(TAG, "MagTester starting — serial companion mode");

    hw_init();

    app_state_t state;
    app_state_init(&state);

    enter_mode(&state, APP_MODE_TEST);

    QueueHandle_t btn_queue = buttons_get_queue();
    button_event_t evt;

    int64_t last_update_us = 0;

    ESP_LOGI(TAG, "Entering main loop");

    while (1) {
        /* ----------------------------------------------------------------
         * Poll for commands from the companion app
         * ---------------------------------------------------------------- */
        int remote_cmd = serial_protocol_recv_cmd();
        if (remote_cmd != 0) {
            /* Inject as a synthetic button event */
            evt.button_num = (uint8_t)(remote_cmd - 1);  /* 1→btn0, 2→btn1 */
            xQueueSend(btn_queue, &evt, 0);
        }

        /* ----------------------------------------------------------------
         * Handle button events (non-blocking poll)
         * ---------------------------------------------------------------- */
        while (xQueueReceive(btn_queue, &evt, 0) == pdTRUE) {
            ESP_LOGD(TAG, "Button %u (mode=%d)", evt.button_num, state.mode);

            if (evt.button_num == 0) {
                if (state.mode == APP_MODE_OPTIONS) {
                    int exit_opts = mode_options_d0(&state);
                    if (exit_opts) {
                        enter_mode(&state, APP_MODE_TEST);
                    }
                } else {
                    cycle_mode(&state);
                }
                serial_protocol_send(&state);
            } else if (evt.button_num == 1) {
                switch (state.mode) {
                    case APP_MODE_LEARN:
                        mode_learn_action(&state);
                        break;
                    case APP_MODE_VERIFY:
                        mode_verify_action(&state);
                        break;
                    case APP_MODE_OPTIONS:
                        mode_options_d1(&state);
                        serial_protocol_send(&state);
                        break;
                    case APP_MODE_TEST:
                    default:
                        break;
                }
            }
        }

        /* ----------------------------------------------------------------
         * Periodic update: read sensors and send state to companion app
         * ---------------------------------------------------------------- */
        int64_t now = esp_timer_get_time();
        if ((now - last_update_us) >= (TEST_UPDATE_MS * 1000LL)) {
            last_update_us = now;

            if (state.mode == APP_MODE_TEST) {
                mode_test_update(&state);
            }

            serial_protocol_send(&state);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
