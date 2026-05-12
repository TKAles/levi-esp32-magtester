#pragma once

#include "app_state.h"

/*
 * Initialize UART0 for serial protocol communication.
 * Installs the RX ring-buffer driver at 115200 8N1.
 * Must be called once during hw_init, after the UART hardware is ready.
 */
void serial_protocol_init(void);

/*
 * Serialize the full application state to a compact JSON line and write it
 * to UART0.  Call this every update cycle (~200 ms) and whenever mode or
 * sub-state changes so the companion app stays in sync.
 */
void serial_protocol_send(const app_state_t *state);

/*
 * Non-blocking poll for a command from the companion app.
 * Returns: 0 = no command, 1 = D0 pressed, 2 = D1 pressed.
 * Commands arrive as plain-text lines: "d0\n" or "d1\n".
 */
int serial_protocol_recv_cmd(void);
