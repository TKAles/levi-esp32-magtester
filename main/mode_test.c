#include "mode_test.h"
#include "mag_sensor.h"
#include "config.h"

#include <string.h>

void mode_test_enter(app_state_t *state)
{
    /* Zero out sensor array so the companion app sees a clean slate */
    memset(state->sensors, 0, sizeof(state->sensors));
}

void mode_test_update(app_state_t *state)
{
    mag_sensor_read_all(state->sensors);
}
