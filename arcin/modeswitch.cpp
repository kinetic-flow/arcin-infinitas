#include <os/time.h>
#include "modeswitch.h"
#include "inf_defines.h"

#define MODE_SWITCH_THRESHOLD_MS 3000

void process_input_mode_switch();
void process_tt_mode_switch();
void process_led_mode_switch();

uint32_t last_capture_time = 0;

uint16_t input_mode_switch_request = 0;
uint16_t tt_mode_switch_request = 0;
uint16_t led_mode_switch_request = 0;

config_flags original_flags = {0};
config_flags current_flags = {0};

bool analog_tt_reverse_direction = false;

config_flags initialize_mode_switch(config_flags flags) {
    original_flags = flags;
    current_flags = original_flags;
    return original_flags;
}

config_flags process_mode_switch(uint16_t raw_input) {
    uint32_t now = Time::time();
    // Capture at most once per 1ms.
    if (now == last_capture_time) {
        return current_flags;
    }
    last_capture_time = now;

    if ((raw_input & ARCIN_PIN_BUTTON_START) &&
        (raw_input & ARCIN_PIN_BUTTON_SELECT)) {

        if (raw_input & ARCIN_PIN_BUTTON_1) {
            // start+sel+1 => input mode switch (controller or keyboard)
            input_mode_switch_request += 1;
        } else if (raw_input & ARCIN_PIN_BUTTON_3) {
            // start+sel+3 => turntable mode switch (analog or digital)
            tt_mode_switch_request += 1;
        } else if (raw_input & ARCIN_PIN_BUTTON_5) {
            // start+sel+5 => LED switch (on or off)
            led_mode_switch_request += 1;
        }

    } else {
        input_mode_switch_request = 0;
        tt_mode_switch_request = 0;
        led_mode_switch_request = 0;
    }

    if (input_mode_switch_request == MODE_SWITCH_THRESHOLD_MS) {
        process_input_mode_switch();
        input_mode_switch_request = 0;
    }

    if (tt_mode_switch_request == MODE_SWITCH_THRESHOLD_MS) {
        process_tt_mode_switch();
        tt_mode_switch_request = 0;
    }

    if (led_mode_switch_request == MODE_SWITCH_THRESHOLD_MS) {
        process_led_mode_switch();
        led_mode_switch_request = 0;
    }
    
    return current_flags;
}

void process_input_mode_switch() {
    uint16_t mode_lights =
        (ARCIN_PIN_BUTTON_START | ARCIN_PIN_BUTTON_SELECT | ARCIN_PIN_BUTTON_1);

    // Controller only => Keyboard only
    if (!current_flags.KeyboardEnable && !current_flags.JoyInputForceDisable) {
        current_flags.KeyboardEnable = 1;
        current_flags.JoyInputForceDisable = 1;
        
        schedule_led(
            2500,
            (mode_lights | ARCIN_PIN_BUTTON_4),
            mode_lights);

        return;
    }
    
    // all modes others => controller only
    current_flags.KeyboardEnable = 0;
    current_flags.JoyInputForceDisable = 0;
    schedule_led(
        2500,
        (mode_lights | ARCIN_PIN_BUTTON_2),
        mode_lights);

    return;
}

void process_tt_mode_switch() {
    uint16_t mode_lights =
        (ARCIN_PIN_BUTTON_START | ARCIN_PIN_BUTTON_SELECT | ARCIN_PIN_BUTTON_3);

    // analog only -> digital only
    if (!current_flags.DigitalTTEnable &&
        !current_flags.AnalogTTForceEnable &&
        !analog_tt_reverse_direction) {

        current_flags.DigitalTTEnable = 1;
        schedule_led(
            2500,
            (mode_lights | ARCIN_PIN_BUTTON_4),
            mode_lights);

        return;
    }

    // digital only -> analog only (flipped)
    if (current_flags.DigitalTTEnable && !current_flags.AnalogTTForceEnable) {
        current_flags.DigitalTTEnable = 0;
        analog_tt_reverse_direction = true;
        schedule_led(
            2500,
            (mode_lights | ARCIN_PIN_BUTTON_6),
            mode_lights);

        return;
    }

    // all others -> analog only
    current_flags.DigitalTTEnable = 0;
    current_flags.AnalogTTForceEnable = 0;
    analog_tt_reverse_direction = false;
    schedule_led(
        2500,
        (mode_lights | ARCIN_PIN_BUTTON_2),
        mode_lights);

    return;
}

void process_led_mode_switch() {
    uint16_t mode_lights =
        (ARCIN_PIN_BUTTON_START | ARCIN_PIN_BUTTON_SELECT | ARCIN_PIN_BUTTON_5);

    // LED off => on
    if (current_flags.LedOff) {
        current_flags.LedOff = 0;
        schedule_led(
            2500,
            (mode_lights | ARCIN_PIN_BUTTON_2),
            mode_lights);

        return;
    }

    // LED on => LED Off
    current_flags.LedOff = 1;
    schedule_led(
        2500,
        (mode_lights | ARCIN_PIN_BUTTON_4),
        mode_lights);

    return;
}