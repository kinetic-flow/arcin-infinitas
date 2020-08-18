#ifndef DEBOUNCE_DEFINES_H
#define DEBOUNCE_DEFINES_H

#include <stdint.h>

uint16_t debounce(uint16_t buttons);

void set_debounce_window(uint8_t new_window);

#endif