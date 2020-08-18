#include <os/time.h>
#include "multifunc.h"
#include "inf_defines.h"

// Window that begins on the first falling edge of E2. At the end of the window
// the number of taps is calculated and the resulting button combination will
// begin to assert.
// i.e., any multi-taps must be done within this window in order to count
#define MULTITAP_DETECTION_WINDOW_MS           500

static uint32_t first_e2_rising_edge_time = 0;

static uint32_t e2_rising_edge_count = 0;

static bool last_e2_status = false;
static uint32_t last_update_time = 0;
void capture_e2_presses(bool pressed) {
    // Update at most once per 1ms.
    if (Time::time() == last_update_time) {
        return;
    }

    last_update_time = Time::time();

    // rising edge (detect off-on sequence)
    if (!last_e2_status && pressed) {
        if (first_e2_rising_edge_time == 0) {
            first_e2_rising_edge_time = last_update_time;
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

    return button;
}

bool is_capture_window_past_due() {
    uint32_t diff;

    // We are not in the capture window
    if (first_e2_rising_edge_time == 0) {
        return false;
    }

    diff = Time::time() - first_e2_rising_edge_time;
    return diff > MULTITAP_DETECTION_WINDOW_MS;
}

static uint16_t effector_buttons_being_asserted = 0;
static uint32_t effector_buttons_assertion_start = 0;
uint16_t get_multi_function_keys(bool is_e2_pressed) {
    if (effector_buttons_assertion_start == 0) {
        capture_e2_presses(is_e2_pressed);
    }

    if (is_e2_pressed && (effector_buttons_assertion_start > 0)) {
        // Keep the time rolling as long as the button is being pressed down
        effector_buttons_assertion_start = Time::time();
    }

    if (is_capture_window_past_due()) {
        effector_buttons_being_asserted = get_multitap_output();
        
        // Reset
        first_e2_rising_edge_time = 0;
        e2_rising_edge_count = 0;

        // Start asserting button combo
        effector_buttons_assertion_start = Time::time();
    }
    
    if (effector_buttons_assertion_start > 0 &&
        (Time::time() - effector_buttons_assertion_start) >= 100) {
        effector_buttons_being_asserted = 0;
        effector_buttons_assertion_start = 0;
    }

    return effector_buttons_being_asserted;
}