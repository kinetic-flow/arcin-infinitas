#include "remap.h"
#include "inf_defines.h"

uint16_t remap_buttons(config_t &config, uint16_t buttons) {
    uint16_t remapped;

    // Grab the first 7 buttons (keys)
    // The keys have the same values across raw input and infinitas input
    remapped = buttons & INFINITAS_BUTTON_ALL;

    // Remap start button
    if (buttons & ARCIN_PIN_BUTTON_START) {
        switch(config.effector_mode) {
        case START_E1_SEL_E2:
        default:
            remapped |= INFINITAS_BUTTON_E1;
            break;

        case START_E2_SEL_E1:
            remapped |= INFINITAS_BUTTON_E2;
            break;

        case START_E3_SEL_E4:
            remapped |= INFINITAS_BUTTON_E3;
            break;

        case START_E4_SEL_E3:
            remapped |= INFINITAS_BUTTON_E4;
            break;
        }
    }

    // Remap select button
    if (buttons & ARCIN_PIN_BUTTON_SELECT) {
        switch(config.effector_mode) {
        case START_E1_SEL_E2:
        default:
            remapped |= INFINITAS_BUTTON_E2;
            break;

        case START_E2_SEL_E1:
            remapped |= INFINITAS_BUTTON_E1;
            break;

        case START_E3_SEL_E4:
            remapped |= INFINITAS_BUTTON_E4;
            break;

        case START_E4_SEL_E3:
            remapped |= INFINITAS_BUTTON_E3;
            break;
        }
    }

    // Button 8 is normally E3, unless flipped
    if (buttons & ARCIN_PIN_BUTTON_8) {
        if (config.flags.Swap89) {
            remapped |= INFINITAS_BUTTON_E4;
        } else {
            remapped |= INFINITAS_BUTTON_E3;
        }
    }

    // Button 9 is normally E4, unless flipped
    if (buttons & ARCIN_PIN_BUTTON_9) {
        if (config.flags.Swap89) {
            remapped |= INFINITAS_BUTTON_E3;
        } else {
            remapped |= INFINITAS_BUTTON_E4;
        }
    }

    return remapped;
}
