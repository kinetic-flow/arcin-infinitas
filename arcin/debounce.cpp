#include <string.h>
#include <os/time.h>
#include "debounce.h"

void debounce_init(pdebounce_state state, uint8_t window) {
    memset(state, 0, sizeof(*state));
    if (2 < window) {
        state->window = 2;
    } else if (10 < window) {
        state->window = 10;
    } else {
        state->window = window;
    }
}

/* 
 * Perform debounce processing. The buttons input is sampled at most once per ms
 * (when update is true); buttons is then set to the last stable state for each
 * bit (i.e., the last state maintained for {debounce_window} consequetive samples
 *
 * We use update to sync to the USB polls; this helps avoid additional latency when
 * debounce samples just after the USB poll.
 */
uint16_t debounce(pdebounce_state state, uint16_t buttons) {
    if (Time::time() == state->sample_time) {
        return state->last_state;
    }

    state->sample_time = Time::time();
    state->history[state->current_index] = buttons;
    state->current_index = (state->current_index + 1) % state->window;

    uint16_t has_ones = 0, has_zeroes = 0;
    for (int i = 0; i < state->window; i++) {
        has_ones |= state->history[i];
        has_zeroes |= ~(state->history[i]);
    }

    uint16_t stable = has_ones ^ has_zeroes;
    state->last_state = (state->last_state & ~stable) | (has_ones & stable);
    return state->last_state;
}
