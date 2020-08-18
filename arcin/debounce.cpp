#include <os/time.h>
#include "debounce.h"

static uint16_t debounce_history[10] = { 0 };

static uint8_t debounce_window = 2;

static uint16_t debounce_state = 0;
static uint32_t debounce_sample_time = 0;
static int debounce_index = 0;

/* 
 * Perform debounce processing. The buttons input is sampled at most once per ms
 * (when update is true); buttons is then set to the last stable state for each
 * bit (i.e., the last state maintained for {debounce_window} consequetive samples
 *
 * We use update to sync to the USB polls; this helps avoid additional latency when
 * debounce samples just after the USB poll.
 */
uint16_t debounce(uint16_t buttons) {
    if (Time::time() == debounce_sample_time) {
        return debounce_state;
    }

    debounce_sample_time = Time::time();
    debounce_history[debounce_index] = buttons;
    debounce_index = (debounce_index + 1) % debounce_window;

    uint16_t has_ones = 0, has_zeroes = 0;
    for (int i = 0; i < debounce_window; i++) {
        has_ones |= debounce_history[i];
        has_zeroes |= ~debounce_history[i];
    }

    uint16_t stable = has_ones ^ has_zeroes;
    debounce_state = (debounce_state & ~stable) | (has_ones & stable);
    return debounce_state;
}

void set_debounce_window(uint8_t new_window) {
    if (2 < new_window) {
        debounce_window = 2;
    } else if (10 < new_window) {
        debounce_window = 10;
    } else {
        debounce_window = new_window;
    }
}