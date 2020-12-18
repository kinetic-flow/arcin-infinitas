#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include "inttypes.h"
#include "color.h"

// Assumes little-endian bit ordering (first bit is LSB)
typedef union _config_flags {
    struct {
        uint32_t SelectMultiFunction: 1;
        uint32_t InvertQE1: 1;

        // was "Swap89" -replaced with E button remapping
        uint32_t Reserved3: 1;

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
        uint32_t LedOff: 1;
        uint32_t TtLedReactive: 1;
        uint32_t TtLedHid: 1;
        uint32_t Ws2812b: 1;
        uint32_t Reserved: 18;
    };

    uint32_t AsUINT32;
} config_flags;

static_assert(sizeof(config_flags) == sizeof(uint32_t), "size mismatch");

typedef union _rgb_config_flags {
    struct {
        uint8_t EnableHidControl: 1;
        uint8_t ReactToTt: 1;
        uint8_t FlipDirection: 1;
        uint8_t Reserved: 5;
    };

    uint8_t AsUINT8;
} rgb_config_flags;

static_assert(sizeof(rgb_config_flags) == sizeof(uint8_t), "size mismatch");

typedef struct _rgb_config {
    rgb_config_flags Flags;
    ColorRgb RgbPrimary;
    uint8_t Darkness;
    ColorRgb RgbSecondary;
    ColorRgb RgbTertiary;
    uint8_t Mode; // WS2812B_Mode
    uint8_t NumberOfLeds;
    int8_t Speed;
} rgb_config;

struct config_t {
    uint8_t label[12];
    config_flags flags;
    int8_t qe1_sens;
    int8_t qe2_sens;

    // was "effector_mode" -replaced with E button remapping
    uint8_t reserved0;

    uint8_t debounce_ticks;

    // [Buttons 1-7] + [E1-E4] + [TT Up] + [TT Down] = 13 + 3pad
    char keycodes[16];

    // upper nibble = start, lower nibble = select
    // 0 = invalid, 1 = E1, 2 = E2, 3 = E3, 4 = E4
    uint8_t remap_start_sel;
    uint8_t remap_b8_b9;

    uint8_t reserved1[2];

    rgb_config rgb;

    uint8_t reserved2[6];
};

// From config_report_t.data[60]
static_assert(sizeof(config_t) == 60, "config size mismatch");

#endif
