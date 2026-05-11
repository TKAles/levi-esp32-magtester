#include "mode_verify.h"
#include "display.h"
#include "mag_sensor.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "mode_verify";

/* -------------------------------------------------------------------------
 * Draw a large pixel-art check-mark starting at (x, y).
 * Drawn as a series of filled rectangles in the given colour.
 * ------------------------------------------------------------------------- */
static void draw_checkmark(int x, int y, uint16_t color, int ps)
{
    /* Right-descending stroke of the tick (lower-left branch) */
    for (int i = 0; i < 5; i++) {
        display_fill_rect(x + i * ps, y + (i + 2) * ps, ps, ps, color);
    }
    /* Rising stroke (upper-right branch) */
    for (int i = 0; i < 8; i++) {
        display_fill_rect(x + (4 + i) * ps, y + (6 - i) * ps, ps, ps, color);
    }
}

/* -------------------------------------------------------------------------
 * Draw a large pixel-art X starting at (x, y), ps = pixel size.
 * ------------------------------------------------------------------------- */
static void draw_cross(int x, int y, uint16_t color, int ps)
{
    for (int i = 0; i < 8; i++) {
        display_fill_rect(x + i * ps,       y + i * ps,       ps, ps, color);
        display_fill_rect(x + (7 - i) * ps, y + i * ps,       ps, ps, color);
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void mode_verify_enter(app_state_t *state)
{
    display_clear(COLOR_BLACK);
    display_draw_string(4, 4, "VERIFY MODE", COLOR_CYAN, COLOR_BLACK, 2);

    if (!state->has_learned_data) {
        display_draw_string(4, 30, "No learned data!", COLOR_RED, COLOR_BLACK, 1);
        display_draw_string(4, 45, "Use LEARN first.", COLOR_WHITE, COLOR_BLACK, 1);
    } else {
        display_draw_string(4, 30, "Press D1 to Verify", COLOR_WHITE, COLOR_BLACK, 1);
        char buf[32];
        snprintf(buf, sizeof(buf), "Hyst:+/-%u  Samp:%u",
                 state->hysteresis, state->num_samples);
        display_draw_string(4, 50, buf, COLOR_CYAN, COLOR_BLACK, 1);
    }
    display_flush();
}

void mode_verify_action(app_state_t *state)
{
    if (!state->has_learned_data) {
        /* Nothing to compare against */
        display_clear(COLOR_BLACK);
        display_draw_string(4, 4, "VERIFY MODE", COLOR_CYAN, COLOR_BLACK, 2);
        display_draw_string(4, 40, "No learned data!", COLOR_RED, COLOR_BLACK, 1);
        display_flush();
        vTaskDelay(pdMS_TO_TICKS(2000));
        mode_verify_enter(state);
        return;
    }

    /* ------------------------------------------------------------------
     * Show measuring banner
     * ------------------------------------------------------------------ */
    display_clear(COLOR_BLACK);
    display_draw_string(4, 4, "VERIFY MODE", COLOR_CYAN, COLOR_BLACK, 2);
    display_draw_string(4, 30, "Measuring...", COLOR_WHITE, COLOR_BLACK, 2);
    display_flush();

    /* ------------------------------------------------------------------
     * Accumulate readings
     * ------------------------------------------------------------------ */
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

    uint8_t averaged[NUM_SENSORS];
    for (int i = 0; i < NUM_SENSORS; i++) {
        averaged[i] = (uint8_t)(accumulator[i] / n);
    }

    /* ------------------------------------------------------------------
     * Compare against learned baseline
     * ------------------------------------------------------------------ */
    uint8_t failed[NUM_SENSORS];
    int     num_failed = 0;

    for (int i = 0; i < NUM_SENSORS; i++) {
        int dev = (int)averaged[i] - (int)state->learned[i];
        if (dev < 0) dev = -dev;
        if ((uint8_t)dev > state->hysteresis) {
            failed[num_failed++] = (uint8_t)i;
        }
    }

    ESP_LOGI(TAG, "Verify: %d/%d sensors failed", num_failed, NUM_SENSORS);

    /* ------------------------------------------------------------------
     * Display result
     * ------------------------------------------------------------------ */
    display_clear(COLOR_BLACK);

    if (num_failed == 0) {
        /* ---- PASS ---- */
        display_draw_string(4, 8, "PASS", COLOR_GREEN, COLOR_BLACK, 4);
        draw_checkmark(190, 10, COLOR_GREEN, 3);
        display_draw_string(4, 45, "All 28 sensors OK", COLOR_GREEN, COLOR_BLACK, 1);
    } else {
        /* ---- FAIL ---- */
        display_draw_string(4, 8, "FAIL", COLOR_RED, COLOR_BLACK, 4);
        draw_cross(190, 8, COLOR_RED, 3);

        /* List failed sensor indices */
        char line1[40] = "Bad sensors: ";
        char line2[40] = "";
        int  chars1 = 13;
        int  on_line2 = 0;

        for (int f = 0; f < num_failed; f++) {
            char tmp[5];
            snprintf(tmp, sizeof(tmp), "%u", failed[f]);
            int tlen = (int)strlen(tmp);

            if (!on_line2 && chars1 + tlen + 1 < (int)sizeof(line1) - 2) {
                if (f > 0) { line1[chars1++] = ','; }
                memcpy(line1 + chars1, tmp, tlen);
                chars1 += tlen;
                line1[chars1] = '\0';
                if (chars1 > 28) on_line2 = 1;
            } else {
                on_line2 = 1;
                if (strlen(line2) + tlen + 2 < sizeof(line2)) {
                    if (strlen(line2) > 0) strcat(line2, ",");
                    strcat(line2, tmp);
                }
            }
        }

        display_draw_string(4, 45, line1, COLOR_WHITE, COLOR_BLACK, 1);
        if (strlen(line2) > 0) {
            display_draw_string(4, 57, line2, COLOR_WHITE, COLOR_BLACK, 1);
        }

        /* Show deviation for first few failed sensors */
        int shown = 0;
        int y_off = 72;
        for (int f = 0; f < num_failed && shown < 5; f++, shown++) {
            uint8_t idx = failed[f];
            int dev = (int)averaged[idx] - (int)state->learned[idx];
            char dbuf[32];
            snprintf(dbuf, sizeof(dbuf), "S%02u: now=%3u ref=%3u",
                     idx, averaged[idx], state->learned[idx]);
            display_draw_string(4, y_off, dbuf, COLOR_YELLOW, COLOR_BLACK, 1);
            y_off += 10;
            (void)dev;
        }
    }

    display_flush();

    /* Hold result; wait 3 s then restore prompt */
    vTaskDelay(pdMS_TO_TICKS(3000));
    mode_verify_enter(state);
}
