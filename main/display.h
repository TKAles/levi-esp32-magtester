#pragma once

#include <stdint.h>
#include "esp_err.h"

/* =========================================================================
 * RGB565 colour macros
 * ========================================================================= */
#define RGB565(r, g, b) ((uint16_t)(((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

#define COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define COLOR_RED        RGB565(255,   0,   0)
#define COLOR_GREEN      RGB565(  0, 255,   0)
#define COLOR_BLUE       RGB565(  0,   0, 255)
#define COLOR_YELLOW     RGB565(255, 255,   0)
#define COLOR_CYAN       RGB565(  0, 255, 255)
#define COLOR_MAGENTA    RGB565(255,   0, 255)
#define COLOR_ORANGE     RGB565(255, 165,   0)
#define COLOR_DARK_GREEN RGB565(  0, 128,   0)
#define COLOR_DARK_RED   RGB565(128,   0,   0)

/* =========================================================================
 * Display API
 * ========================================================================= */

/**
 * @brief Initialise the ST7789 panel via esp_lcd and set up the framebuffer.
 */
esp_err_t display_init(void);

/**
 * @brief Fill the entire framebuffer with a solid colour.
 */
void display_clear(uint16_t color);

/**
 * @brief Fill a rectangle in the framebuffer.
 */
void display_fill_rect(int x, int y, int w, int h, uint16_t color);

/**
 * @brief Draw a single character at (x, y) with the given scale factor.
 *
 * @param x     Left edge pixel
 * @param y     Top edge pixel
 * @param ch    ASCII character (0x20–0x7E)
 * @param fg    Foreground colour (RGB565)
 * @param bg    Background colour (RGB565)
 * @param scale Pixel scale (1 = 8×8 px, 2 = 16×16 px, …)
 */
void display_draw_char(int x, int y, char ch, uint16_t fg, uint16_t bg, int scale);

/**
 * @brief Draw a null-terminated string starting at (x, y).
 */
void display_draw_string(int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale);

/**
 * @brief Push the framebuffer to the display.
 */
void display_flush(void);
