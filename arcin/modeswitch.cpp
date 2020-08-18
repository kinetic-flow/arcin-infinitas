#include <os/time.h>
#include "modeswitch.h"
#include "inf_defines.h"

#define MODE_SWITCH_THRESHOLD_MS 3000

void process_input_mode_switch();
void process_tt_mode_switch();

uint32_t last_capture_time = 0;

uint16_t input_mode_switch_request = 0;
uint16_t tt_mode_switch_request = 0;

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

        if (raw_input & ARCIN_PIN_BUTTON_1) {
            // start+sel+1 => input mode switch (controller or keyboard)
            input_mode_switch_request += 1;
        }else if (raw_input & ARCIN_PIN_BUTTON_3) {
            // start+sel+3 => turntable mode switch (analog or digital)
            tt_mode_switch_request += 1;
        }

    } else {
        input_mode_switch_request = 0;
        tt_mode_switch_request = 0;
    }

    if (input_mode_switch_request == MODE_SWITCH_THRESHOLD_MS) {
        process_input_mode_switch();
        input_mode_switch_request = 0;
    }

    if (tt_mode_switch_request == MODE_SWITCH_THRESHOLD_MS) {
        process_tt_mode_switch();
        input_mode_switch_request = 0;
    }
    
    return current_flags;
}

void process_input_mode_switch() {
    uint16_t mode_lights =
        (ARCIN_PIN_BUTTON_START | ARCIN_PIN_BUTTON_SELECT | ARCIN_PIN_BUTTON_1);

    // both => controller only
    if (current_flags.KeyboardEnable && current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 0;
        current_flags.JoyInputForceDisable = 0;
        
        schedule_led(
            Time::time() + 2500,
            (mode_lights | ARCIN_PIN_BUTTON_2),
            mode_lights);

        return;
    }

    // Controller only => Keyboard only
    if (!current_flags.KeyboardEnable && !current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 1;
        current_flags.JoyInputForceDisable = 1;
        
        schedule_led(
            Time::time() + 2500,
            (mode_lights | ARCIN_PIN_BUTTON_4),
            mode_lights);

        return;
    }

    // Keyboard only => Controller only
    if (current_flags.KeyboardEnable && current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 0;
        current_flags.JoyInputForceDisable = 0;

        schedule_led(
            Time::time() + 2500,
            (mode_lights | ARCIN_PIN_BUTTON_2),
            mode_lights);

        return;
    }
}

void process_tt_mode_switch() {
    uint16_t mode_lights =
        (ARCIN_PIN_BUTTON_START | ARCIN_PIN_BUTTON_SELECT | ARCIN_PIN_BUTTON_3);

    // both -> analog only
    if (current_flags.AnalogTTForceEnable) {
        current_flags.DigitalTTEnable = 0;
        current_flags.AnalogTTForceEnable = 0;
        schedule_led(
            Time::time() + 2500,
            (mode_lights | ARCIN_PIN_BUTTON_2),
            mode_lights);

        return;
    }

    // analog only -> digital only
    if (!current_flags.DigitalTTEnable) {
        current_flags.DigitalTTEnable = 1;
        current_flags.AnalogTTForceEnable = 0;
        schedule_led(
            Time::time() + 2500,
            (mode_lights | ARCIN_PIN_BUTTON_4),
            mode_lights);

        return;
    }

    // digital only -> analog only
    if (current_flags.DigitalTTEnable) {
        current_flags.DigitalTTEnable = 0;
        current_flags.AnalogTTForceEnable = 0;
        schedule_led(
            Time::time() + 2500,
            (mode_lights | ARCIN_PIN_BUTTON_2),
            mode_lights);

        return;
    }
}