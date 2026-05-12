#include "serial_protocol.h"
#include "config.h"

#include "driver/uart.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>

#define SP_UART_NUM     UART_NUM_0
#define SP_BAUD_RATE    115200
#define SP_RX_BUF_SIZE  512

static const char *TAG = "serial_proto";

/* Accumulated line buffer for incoming commands */
static char     s_rx_buf[128];
static int      s_rx_pos = 0;

/* =========================================================================
 * Initialisation
 * ========================================================================= */

void serial_protocol_init(void)
{
    uart_config_t cfg = {
        .baud_rate           = SP_BAUD_RATE,
        .data_bits           = UART_DATA_8_BITS,
        .parity              = UART_PARITY_DISABLE,
        .stop_bits           = UART_STOP_BITS_1,
        .flow_ctrl           = UART_HW_FLOWCTRL_DISABLE,
        .source_clk          = UART_SCLK_DEFAULT,
    };

    uart_param_config(SP_UART_NUM, &cfg);

    /* Install RX ring-buffer; TX buffer = 0 so writes go directly to the FIFO */
    esp_err_t ret = uart_driver_install(SP_UART_NUM, SP_RX_BUF_SIZE, 0, 0, NULL, 0);
    if (ret == ESP_ERR_INVALID_STATE) {
        /* Driver already installed by the VFS console layer — RX will still work */
        ESP_LOGW(TAG, "UART driver already installed, continuing");
    } else {
        ESP_ERROR_CHECK(ret);
    }

    ESP_LOGI(TAG, "Serial protocol ready at %d baud", SP_BAUD_RATE);
}

/* =========================================================================
 * State serialisation → JSON
 * ========================================================================= */

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

    /* Failed sensor indices */
    APPEND(",\"vb\":[");
    for (int i = 0; i < state->verify_num_bad; i++) {
        APPEND("%u%s", state->verify_bad[i], (i < state->verify_num_bad - 1) ? "," : "");
    }
    APPEND("]");

    /* Averaged readings for failed sensors */
    APPEND(",\"va\":[");
    for (int i = 0; i < state->verify_num_bad; i++) {
        uint8_t idx = state->verify_bad[i];
        APPEND("%u%s", state->verify_averaged[idx], (i < state->verify_num_bad - 1) ? "," : "");
    }
    APPEND("]");

    /* Learned (reference) values for failed sensors */
    APPEND(",\"lb\":[");
    for (int i = 0; i < state->verify_num_bad; i++) {
        uint8_t idx = state->verify_bad[i];
        APPEND("%u%s", state->learned[idx], (i < state->verify_num_bad - 1) ? "," : "");
    }
    APPEND("]}\n");

#undef APPEND

    uart_write_bytes(SP_UART_NUM, pkt, n);
}

/* =========================================================================
 * Command receive
 * ========================================================================= */

int serial_protocol_recv_cmd(void)
{
    uint8_t ch;
    int     cmd = 0;

    while (uart_read_bytes(SP_UART_NUM, &ch, 1, 0) == 1) {
        if (ch == '\n' || ch == '\r') {
            if (s_rx_pos > 0) {
                s_rx_buf[s_rx_pos] = '\0';
                if (strcmp(s_rx_buf, "d0") == 0) {
                    cmd = 1;
                } else if (strcmp(s_rx_buf, "d1") == 0) {
                    cmd = 2;
                }
                s_rx_pos = 0;
            }
        } else if (s_rx_pos < (int)sizeof(s_rx_buf) - 1) {
            s_rx_buf[s_rx_pos++] = (char)ch;
        }
    }

    return cmd;
}
