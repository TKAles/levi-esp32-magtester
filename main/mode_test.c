#include "mode_test.h"
#include "display.h"
#include "mag_sensor.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Layout constants for the 7x4 sensor grid
 *
 * Grid: 7 columns (ADCs) x 4 rows (channels)
 * Cell size: 28x28 pixels, 4-px gap between cells
 * Grid starts at x=8, y=24 (leaving room for header)
 * ------------------------------------------------------------------------- */
#define CELL_W       28
#define CELL_H       28
#define CELL_GAP     4
#define GRID_X       8
#define GRID_Y       24

/* Return an RGB565 colour representing how far `val` is from ADC_CENTER */
static uint16_t value_to_color(uint8_t val)
{
    int dev = (int)val - ADC_CENTER;   /* -128 ... +127 */
    if (dev < 0) dev = -dev;           /* absolute deviation */

    if (dev < 5)  return COLOR_DARK_GREEN;   /* near centre -> dark green */
    if (dev < 15) return COLOR_GREEN;
    if (dev < 30) return COLOR_YELLOW;
    if (dev < 50) return COLOR_ORANGE;
    if (dev < 80) return COLOR_RED;
    return COLOR_DARK_RED;
}

void mode_test_enter(app_state_t *state)
{
    (void)state;
    display_clear(COLOR_BLACK);
    display_draw_string(2, 4, "TEST MODE  7x4 Hall Sensors",
                        COLOR_CYAN, COLOR_BLACK, 1);
    display_flush();
}

void mode_test_update(app_state_t *state)
{
    (void)state;
    uint8_t readings[NUM_SENSORS];
    mag_sensor_read_all(readings);

    /* Redraw grid area only */
    display_fill_rect(0, 16, TFT_WIDTH, TFT_HEIGHT - 16, COLOR_BLACK);

    for (int adc = 0; adc < NUM_ADCS; adc++) {          /* columns */
        for (int ch = 0; ch < CHANNELS_PER_ADC; ch++) { /* rows */
            int idx = adc * CHANNELS_PER_ADC + ch;
            uint8_t val = readings[idx];

            int cx = GRID_X + adc * (CELL_W + CELL_GAP);
            int cy = GRID_Y + ch  * (CELL_H + CELL_GAP);

            uint16_t bg = value_to_color(val);
            display_fill_rect(cx, cy, CELL_W, CELL_H, bg);

            /* Print 3-digit value centered inside cell */
            char buf[4];
            snprintf(buf, sizeof(buf), "%3u", val);
            /* 3 chars x 8 px = 24 px wide; offset to center in 28-px cell */
            display_draw_string(cx + 2, cy + 10, buf,
                                COLOR_WHITE, bg, 1);
        }
    }

    /* Draw ADC column labels (top) */
    for (int adc = 0; adc < NUM_ADCS; adc++) {
        char lbl[3];
        snprintf(lbl, sizeof(lbl), "%d", adc + 1);
        int lx = GRID_X + adc * (CELL_W + CELL_GAP) + 10;
        display_draw_string(lx, GRID_Y - 10, lbl, COLOR_CYAN, COLOR_BLACK, 1);
    }

    display_flush();
}
