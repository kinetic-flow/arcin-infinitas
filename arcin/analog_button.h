#ifndef ANALOG_BUTTON_DEFINES_H
#define ANALOG_BUTTON_DEFINES_H

#include <stdint.h>
#include <os/time.h>

class analog_button {
public:
    // config

    // Number of ticks we need to advance before recognizing an input
    uint32_t deadzone;
    // How long to sustain the input before clearing it (if opposite direction is input, we'll release immediately)
    uint32_t sustain_ms;
    // Always provide a zero-input for one poll before reversing?
    bool clear;

    // State: Center of deadzone
    uint32_t center;
    bool center_valid;
    // times to: reset to zero, reset center to counter
    uint32_t t_timeout;

    int8_t state; // -1, 0, 1
    int8_t last_delta;
public:
    analog_button(uint32_t deadzone, uint32_t sustain_ms, bool clear)
        : deadzone(deadzone), sustain_ms(sustain_ms), clear(clear)
    {
        center = 0;
        center_valid = false;
        t_timeout = 0;
        state = 0;
    }

    int8_t poll(uint32_t current_value) {
        if (!center_valid) {
            center_valid = true;
            center = current_value;
        }

        uint8_t observed = current_value;
        int8_t delta = observed - center;
        last_delta = delta;

        uint8_t direction = 0;
        if (delta >= (int32_t)deadzone) {
            direction = 1;
        } else if (delta <= -(int32_t)deadzone) {
            direction = -1;
        }

        if (direction != 0) {
            center = observed;
            t_timeout = Time::time() + sustain_ms;
        } else if (t_timeout != 0 && Time::time() >= t_timeout) {
            state = 0;
            center = observed;
            t_timeout = 0;
        }

        if (direction == -state && clear) {
            state = direction;
            return 0;
        } else if (direction != 0) {
            state = direction;
        }

        return state;
    }
};

#endif
