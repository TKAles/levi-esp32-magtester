#include "serial_protocol.h"
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "serial_proto";

/* Queue through which the RX task delivers parsed commands to the main loop */
static QueueHandle_t s_cmd_queue;

/* =========================================================================
 * RX task — reads lines from stdin (USB Serial JTAG console) and enqueues
 * button commands sent by the companion app.
 * ========================================================================= */

static void rx_task(void *arg)
{
    char buf[32];
    int  pos = 0;

    while (1) {
        int ch = fgetc(stdin);

        if (ch < 0) {
            /* No data yet — yield so other tasks can run */
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if ((char)ch == '\n' || (char)ch == '\r') {
            if (pos > 0) {
                buf[pos] = '\0';
                uint8_t cmd = 0;
                if      (strcmp(buf, "d0") == 0) cmd = 1;
                else if (strcmp(buf, "d1") == 0) cmd = 2;
                if (cmd != 0) {
                    xQueueSend(s_cmd_queue, &cmd, 0);
                }
                pos = 0;
            }
        } else if (pos < (int)sizeof(buf) - 1) {
            buf[pos++] = (char)ch;
        }
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void serial_protocol_init(void)
{
    s_cmd_queue = xQueueCreate(8, sizeof(uint8_t));
    xTaskCreate(rx_task, "serial_rx", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Serial protocol ready on USB console");
}

void serial_protocol_send(const app_state_t *state)
{
    static char pkt[640];
    int n = 0;

#define APPEND(fmt, ...) n += snprintf(pkt + n, (int)sizeof(pkt) - n, fmt, ##__VA_ARGS__)

    APPEND("{\"m\":%u,\"s\":[", state->mode);
    for (int i = 0; i < NUM_SENSORS; i++) {
        APPEND("%u%s", state->sensors[i], (i < NUM_SENSORS - 1) ? "," : "");
    }
    APPEND("],\"hd\":%u,\"hy\":%u,\"ns\":%u,\"oc\":%u,\"ls\":%u,\"vs\":%u,\"vn\":%u",
           state->has_learned_data,
           state->hysteresis,
           state->num_samples,
           state->options_cursor,
           state->learn_sub,
           state->verify_sub,
           state->verify_num_bad);

    APPEND(",\"vb\":[");
    for (int i = 0; i < state->verify_num_bad; i++) {
        APPEND("%u%s", state->verify_bad[i], (i < state->verify_num_bad - 1) ? "," : "");
    }
    APPEND("]");

    APPEND(",\"va\":[");
    for (int i = 0; i < state->verify_num_bad; i++) {
        uint8_t idx = state->verify_bad[i];
        APPEND("%u%s", state->verify_averaged[idx], (i < state->verify_num_bad - 1) ? "," : "");
    }
    APPEND("]");

    APPEND(",\"lb\":[");
    for (int i = 0; i < state->verify_num_bad; i++) {
        uint8_t idx = state->verify_bad[i];
        APPEND("%u%s", state->learned[idx], (i < state->verify_num_bad - 1) ? "," : "");
    }
    APPEND("]}");

#undef APPEND

    pkt[n] = '\0';
    /* printf routes through the IDF console VFS (USB Serial JTAG) */
    puts(pkt);   /* puts appends '\n' and is slightly faster than printf */
    fflush(stdout);
}

int serial_protocol_recv_cmd(void)
{
    uint8_t cmd = 0;
    xQueueReceive(s_cmd_queue, &cmd, 0);
    return (int)cmd;
}
