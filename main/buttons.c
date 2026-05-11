#include "buttons.h"
#include "config.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG = "buttons";

/* Queue depth — enough to buffer a few fast presses */
#define BUTTON_QUEUE_DEPTH 8

static QueueHandle_t s_btn_queue = NULL;

/* Track last interrupt time per button for debounce */
static volatile int64_t s_last_isr_us[2] = { 0, 0 };

/* =========================================================================
 * ISR handler
 * ========================================================================= */

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t btn = (uint32_t)(uintptr_t)arg;

    /* Read GPIO level — we only care about falling edge (button pressed) */
    int level = gpio_get_level((gpio_num_t)(btn == 0 ? BUTTON_D0_GPIO : BUTTON_D1_GPIO));
    if (level != 0) {
        /* Rising edge — ignore (release) */
        return;
    }

    int64_t now = esp_timer_get_time();  /* us */
    if ((now - s_last_isr_us[btn]) < (BUTTON_DEBOUNCE_MS * 1000LL)) {
        /* Within debounce window — ignore */
        return;
    }
    s_last_isr_us[btn] = now;

    button_event_t evt = {
        .button_num = (uint8_t)btn,
        .press_type = PRESS_TYPE_SHORT,
    };

    BaseType_t higher_prio_woken = pdFALSE;
    xQueueSendFromISR(s_btn_queue, &evt, &higher_prio_woken);
    if (higher_prio_woken) {
        portYIELD_FROM_ISR();
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

esp_err_t buttons_init(void)
{
    s_btn_queue = xQueueCreate(BUTTON_QUEUE_DEPTH, sizeof(button_event_t));
    if (!s_btn_queue) {
        ESP_LOGE(TAG, "Failed to create button queue");
        return ESP_ERR_NO_MEM;
    }

    /* Configure both button GPIOs as inputs with internal pull-ups.
     * Buttons pull GPIO to GND when pressed (active-low). */
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_D0_GPIO) | (1ULL << BUTTON_D1_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    esp_err_t ret = gpio_config(&io_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Install the GPIO ISR service if not already done */
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        /* ESP_ERR_INVALID_STATE means already installed — that's fine */
        ESP_LOGE(TAG, "gpio_install_isr_service failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Add per-pin ISR handlers */
    gpio_isr_handler_add(BUTTON_D0_GPIO, gpio_isr_handler, (void *)0);
    gpio_isr_handler_add(BUTTON_D1_GPIO, gpio_isr_handler, (void *)1);

    ESP_LOGI(TAG, "Buttons initialised (D0=GPIO%d, D1=GPIO%d)",
             BUTTON_D0_GPIO, BUTTON_D1_GPIO);
    return ESP_OK;
}

QueueHandle_t buttons_get_queue(void)
{
    return s_btn_queue;
}
