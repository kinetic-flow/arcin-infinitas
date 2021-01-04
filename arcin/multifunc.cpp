#include "timer.h"
#include "multifunc.h"
#include "inf_defines.h"

extern uint32_t debug_value;

// Window that begins on the first rising edge of E2
// i.e., any multi-taps must be done within this window in order to count
#define MULTITAP_DETECTION_WINDOW_MS 500

// assert button combination for this duration
#define EFFECTOR_COMBO_HOLD_DURATION_MS 100

typedef enum _MF_CURRENT_WINDOW {
    MF_IDLE,
    MF_CAPTURING_INPUT,
    MF_ASSERTING_INPUT
} MF_CURRENT_WINDOW;

static MF_CURRENT_WINDOW current_window = MF_IDLE;

timer window_close_timer;

// how many times was E2 pressed during the capture window?
static uint8_t e2_rising_edge_count = 0;

static bool last_e2_status = false;
void capture_e2_presses(bool pressed) {
    // rising edge (detect off-on sequence)
    if (!last_e2_status && pressed) {

        // if we were idle, start capturing input now.
        if (current_window == MF_IDLE) {
            e2_rising_edge_count = 0;
            window_close_timer.arm(MULTITAP_DETECTION_WINDOW_MS);
            current_window = MF_CAPTURING_INPUT;
        }

        // count every rising edge
        e2_rising_edge_count += 1;
    }

    last_e2_status = pressed;
}

uint16_t get_multitap_output(uint8_t num_rising_edges) {
    uint16_t button;
    switch (num_rising_edges) {
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

// used as a limiter
static uint32_t last_update_time = 0;

// button combination to assert
static uint16_t effector_buttons_being_asserted = 0;

uint16_t get_multi_function_keys(bool is_e2_pressed) {
    uint32_t now = Time::time();

    // Update at most once per 1ms. Otherwise, just return the last calculated
    // result.
    if (now == last_update_time) {
        return effector_buttons_being_asserted;
    }
    last_update_time = now;

    // capture button presses
    if (current_window == MF_IDLE || current_window == MF_CAPTURING_INPUT) {
        capture_e2_presses(is_e2_pressed);
    }

    // are we past capture window?
    if (current_window == MF_CAPTURING_INPUT && window_close_timer.check_expiry_and_reset()) {
        // Start asserting button combo
        current_window = MF_ASSERTING_INPUT;
        window_close_timer.arm(EFFECTOR_COMBO_HOLD_DURATION_MS);
        effector_buttons_being_asserted = get_multitap_output(e2_rising_edge_count);
    }

    // are we past assertion window?
    if (current_window == MF_ASSERTING_INPUT && window_close_timer.check_expiry_and_reset()) {
        if (is_e2_pressed) {
            // If the button is held down, extend the timer
            window_close_timer.arm(EFFECTOR_COMBO_HOLD_DURATION_MS);
        } else {
            current_window = MF_IDLE;
            effector_buttons_being_asserted = 0;
        }
    }

    return effector_buttons_being_asserted;
}