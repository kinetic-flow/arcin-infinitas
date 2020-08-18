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
        // start and select are not being held
        input_mode_switch_request = 0;
    }

    if (input_mode_switch_request >= 3000) {
        input_mode_switch_request = 0;
        process_input_mode_switch();
    }
    
    return current_flags;
}

void process_input_mode_switch() {
    // Controller only => Keyboard only
    if (!current_flags.KeyboardEnable && !current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 1;
        current_flags.JoyInputForceDisable = 1;
        return;
    }

    // Keyboard only => Controller only
    if (current_flags.KeyboardEnable && current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 0;
        current_flags.JoyInputForceDisable = 0;
        return;
    }

    // "Both" state is not used here; I doubt many people actually use it.
}