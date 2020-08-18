#include <os/time.h>
#include "modeswitch.h"
#include "inf_defines.h"

void process_input_mode_switch();

uint32_t last_capture_time = 0;

uint16_t input_mode_switch_request = 0;

config_flags original_flags = {0};
config_flags current_flags = {0};

config_flags initialize_mode_switch(config_flags flags) {
    original_flags = flags;
    current_flags = original_flags;
    return original_flags;
}

config_flags process_mode_switch(uint16_t raw_input) {
    // Capture at most once per 1ms.
    if (Time::time() == last_capture_time) {
        return current_flags;
    }
    last_capture_time = Time::time();

    if ((raw_input & ARCIN_PIN_BUTTON_START) &&
        (raw_input & ARCIN_PIN_BUTTON_SELECT)) {

        // start+sel+2 => input mode switch (controller, keyboard, both)
        if (raw_input & ARCIN_PIN_BUTTON_2) {
            input_mode_switch_request += 1;
        }

    } else {
        input_mode_switch_request = 0;
    }

    // start and select are not being held
    if (input_mode_switch_request == 3000) {
        process_input_mode_switch();
        input_mode_switch_request = 0;
    }
    
    return current_flags;
}

void process_input_mode_switch() {

    // Controller only => Keyboard only
    if (!current_flags.KeyboardEnable && !current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 1;
        current_flags.JoyInputForceDisable = 1;
        
        schedule_led(
            Time::time() + 2000,
            (ARCIN_PIN_BUTTON_START | ARCIN_PIN_BUTTON_SELECT |
             ARCIN_PIN_BUTTON_2 | ARCIN_PIN_BUTTON_3));

        return;
    }

    // Keyboard only => Controller only
    if (current_flags.KeyboardEnable && current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 0;
        current_flags.JoyInputForceDisable = 0;

        schedule_led(
            Time::time() + 2000,
            (ARCIN_PIN_BUTTON_START | ARCIN_PIN_BUTTON_SELECT |
             ARCIN_PIN_BUTTON_2 | ARCIN_PIN_BUTTON_1));

        return;
    }

    // "Both" state is not used here; I doubt many people actually use it.

}