#include <os/time.h>
#include "multifunc.h"
#include "inf_defines.h"

// Window that begins on the first falling edge of E2. At the end of the window
// the number of taps is calculated and the resulting button combination will
// begin to assert.
// i.e., any multi-taps must be done within this window in order to count
#define MULTITAP_DETECTION_WINDOW_MS           500

static uint32_t first_e2_rising_edge_time;

static bool last_e2_status;
static uint32_t e2_rising_edge_count;

void e2_update(bool pressed) {
    // rising edge (detect off-on sequence)
    if (!last_e2_status && pressed) {
        if (e2_rising_edge_count == 0) {
            first_e2_rising_edge_time = Time::time();
        }

        e2_rising_edge_count += 1;
    }

    last_e2_status = pressed;
}

uint16_t get_multitap_output() {
    uint16_t button;
    switch (e2_rising_edge_count) {
    case 0:
        button = 0;
        break;

    case 1:
        button = INFINITAS_BUTTON_E2;
        break;

    case 2:
        button = INFINITAS_BUTTON_E3;
        break;

    case 3:
        button = (INFINITAS_BUTTON_E2 | INFINITAS_BUTTON_E3);
        break;

    case 4:
    default:
        button = INFINITAS_BUTTON_E4;
        break;
    }

    // Reset
    e2_rising_edge_count = 0;
    return button;
}

bool is_multitap_window_closed() {
    uint32_t diff;

    if (e2_rising_edge_count == 0) {
        return false;
    }

    diff = Time::time() - first_e2_rising_edge_time;
    return diff > MULTITAP_DETECTION_WINDOW_MS;
}