#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Assumes little-endian bit ordering (first bit is LSB)
typedef union _config_flags {
    struct {
        uint32_t SelectMultiFunction: 1;
        uint32_t InvertQE1: 1;
        uint32_t Swap89: 1;
        uint32_t DigitalTTEnable: 1;
        uint32_t DebounceEnable: 1;
        uint32_t PollAt250Hz: 1;
        
        // Normally, DigitalTTEnable causes the analog turntable to be disabled.
        // With this flag, analog can also be enabled.
        // This is obtuse on purpose to keep binary compatibility with
        // configuration saved by older firmware!

        uint32_t AnalogTTForceEnable: 1;
        uint32_t KeyboardEnable: 1;
        uint32_t JoyInputForceDisable: 1;
        uint32_t ModeSwitchEnable: 1;
        uint32_t Reserved: 22;
    };

    uint32_t AsUINT32;
} config_flags, *pconfig_flags;

static_assert(sizeof(config_flags) == sizeof(uint32_t), "size mismatch");

struct config_t {
    uint8_t label[12];
    config_flags flags;
    int8_t qe1_sens;
    int8_t qe2_sens;
    uint8_t effector_mode;
    uint8_t debounce_ticks;

    // [Buttons 1-7] + [E1-E4] + [TT Up] + [TT Down] = 13 + 3pad
    char keycodes[16];

    uint8_t reserved[24];
};

// From config_report_t.data[60]
static_assert(sizeof(config_t) == 60, "config size mismatch");

typedef enum _effector_mode_option {
    START_E1_SEL_E2 = 0,
    START_E2_SEL_E1,
    START_E3_SEL_E4,
    START_E4_SEL_E3
} effector_mode_option, *peffector_mode_option;

#endif
