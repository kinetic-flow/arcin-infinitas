#ifndef DEBOUNCE_DEFINES_H
#define DEBOUNCE_DEFINES_H

#include <stdint.h>

typedef struct _debounce_state {
    uint16_t history[10];
    uint8_t window;
    uint16_t last_state;
    uint32_t sample_time;
    int current_index;
} debounce_state, *pdebounce_state;

void debounce_init(pdebounce_state state, uint8_t window);

uint16_t debounce(pdebounce_state state, uint16_t buttons);

#endif