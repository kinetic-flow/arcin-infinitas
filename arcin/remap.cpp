#include "remap.h"
#include "inf_defines.h"

uint16_t get_effector_button(uint8_t effector_number) {
    switch(effector_number) {
    case 1:
        return INFINITAS_BUTTON_E1;

    case 2:
        return INFINITAS_BUTTON_E2;

    case 3:
        return INFINITAS_BUTTON_E3;

    case 4:
        return INFINITAS_BUTTON_E4;

    default:
        return 0;
    }
}

uint16_t get_effector(uint8_t effector_number, uint16_t default_button) {
    uint16_t button = get_effector_button(effector_number);
    if (button == 0) {
        button = default_button;
    }
    return button;
}

uint16_t remap_buttons(config_t &config, uint16_t buttons) {
    uint16_t remapped;

    // Grab the first 7 buttons (keys)
    // The keys have the same values across raw input and infinitas input
    remapped = buttons & INFINITAS_BUTTON_ALL;

    // Remap effectors
    if (buttons & ARCIN_PIN_BUTTON_START) {
        remapped |= get_effector((config.remap_start_sel >> 4) & 0xF, INFINITAS_BUTTON_E1);
    }
    if (buttons & ARCIN_PIN_BUTTON_SELECT) {
        remapped |= get_effector((config.remap_start_sel) & 0xF, INFINITAS_BUTTON_E2);
    }
    if (buttons & ARCIN_PIN_BUTTON_8) {
        remapped |= get_effector((config.remap_b8_b9 >> 4) & 0xF, INFINITAS_BUTTON_E3);
    }
    if (buttons & ARCIN_PIN_BUTTON_9) {
        remapped |= get_effector((config.remap_b8_b9) & 0xF, INFINITAS_BUTTON_E4);
    }

    return remapped;
}
