#include "display.h"
#include "config.h"
#include "font8x8.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include <string.h>
#include <stdlib.h>

static const char *TAG = "display";

/* LCD panel handle obtained from esp_lcd */
static esp_lcd_panel_handle_t s_panel   = NULL;
static esp_lcd_panel_io_handle_t s_io   = NULL;

/* Full-screen framebuffer in DRAM */
static uint16_t *s_fb = NULL;

/* =========================================================================
 * Initialisation
 * ========================================================================= */

esp_err_t display_init(void)
{
    /* -----------------------------------------------------------------
     * Allocate framebuffer
     * ----------------------------------------------------------------- */
    s_fb = (uint16_t *)heap_caps_malloc(TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t),
                                         MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!s_fb) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer (%u bytes)",
                 (unsigned)(TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t)));
        return ESP_ERR_NO_MEM;
    }
    memset(s_fb, 0, TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t));

    /* -----------------------------------------------------------------
     * Configure back-light GPIO
     * ----------------------------------------------------------------- */
    gpio_config_t bl_cfg = {
        .pin_bit_mask = (1ULL << TFT_BL_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&bl_cfg);
    gpio_set_level(TFT_BL_GPIO, 1);   /* backlight on */

    /* -----------------------------------------------------------------
     * Create LCD panel IO (SPI)
     * The TFT shares SPI2_HOST with the ADC bus but uses hardware CS via
     * the esp_lcd driver.
     * ----------------------------------------------------------------- */
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num       = TFT_DC_GPIO,
        .cs_gpio_num       = TFT_CS_GPIO,
        .pclk_hz           = TFT_SPI_CLOCK_HZ,
        .lcd_cmd_bits      = 8,
        .lcd_param_bits    = 8,
        .spi_mode          = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx          = NULL,
        .flags = {
            .dc_low_on_data = 0,
            .octal_mode     = 0,
            .sio_mode       = 0,
            .lsb_first      = 0,
            .cs_high_active = 0,
        },
    };

    esp_err_t ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,
                                              &io_cfg, &s_io);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_spi: %s", esp_err_to_name(ret));
        return ret;
    }

    /* -----------------------------------------------------------------
     * Create ST7789 panel
     * ----------------------------------------------------------------- */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num   = TFT_RST_GPIO,
        .rgb_endian       = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel   = 16,
        .flags = {
            .reset_active_high = 0,
        },
        .vendor_config    = NULL,
    };

    ret = esp_lcd_new_panel_st7789(s_io, &panel_cfg, &s_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_st7789: %s", esp_err_to_name(ret));
        return ret;
    }

    /* -----------------------------------------------------------------
     * Hardware reset + init
     * ----------------------------------------------------------------- */
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);

    /* Configure for Adafruit ESP32-S3 Reverse TFT Feather (ST7789 240x135) */
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_set_gap(s_panel, TFT_GAP_X, TFT_GAP_Y);
    esp_lcd_panel_swap_xy(s_panel, true);
    esp_lcd_panel_mirror(s_panel, true, false);
    esp_lcd_panel_disp_on_off(s_panel, true);

    ESP_LOGI(TAG, "ST7789 display initialised (%dx%d)", TFT_WIDTH, TFT_HEIGHT);
    return ESP_OK;
}

/* =========================================================================
 * Drawing primitives (operate on framebuffer)
 * ========================================================================= */

void display_clear(uint16_t color)
{
    int total = TFT_WIDTH * TFT_HEIGHT;
    for (int i = 0; i < total; i++) {
        s_fb[i] = color;
    }
}

void display_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    /* Clamp to screen bounds */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH  - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    for (int row = y; row < y + h; row++) {
        uint16_t *ptr = s_fb + row * TFT_WIDTH + x;
        for (int col = 0; col < w; col++) {
            ptr[col] = color;
        }
    }
}

void display_draw_char(int x, int y, char ch, uint16_t fg, uint16_t bg, int scale)
{
    uint8_t idx = (uint8_t)ch;
    if (idx < 0x20 || idx > 0x7E) {
        idx = 0x20; /* render unknown chars as space */
    }
    idx -= 0x20;

    const uint8_t *bitmap = font8x8_basic[idx];

    for (int row = 0; row < 8; row++) {
        uint8_t bits = bitmap[row];
        for (int col = 0; col < 8; col++) {
            uint16_t pixel = (bits & (0x80 >> col)) ? fg : bg;
            /* Draw scaled pixel block */
            for (int sy = 0; sy < scale; sy++) {
                int py = y + row * scale + sy;
                if (py < 0 || py >= TFT_HEIGHT) continue;
                for (int sx = 0; sx < scale; sx++) {
                    int px = x + col * scale + sx;
                    if (px < 0 || px >= TFT_WIDTH) continue;
                    s_fb[py * TFT_WIDTH + px] = pixel;
                }
            }
        }
    }
}

void display_draw_string(int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale)
{
    if (!str) return;
    int cx = x;
    while (*str) {
        display_draw_char(cx, y, *str, fg, bg, scale);
        cx += 8 * scale;
        str++;
    }
}

/* =========================================================================
 * Flush framebuffer → display
 * ========================================================================= */

void display_flush(void)
{
    esp_lcd_panel_draw_bitmap(s_panel,
                               0, 0,
                               TFT_WIDTH, TFT_HEIGHT,
                               s_fb);
}
