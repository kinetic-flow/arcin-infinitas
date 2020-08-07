#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

struct config_t {
    uint8_t label[12];
    uint32_t flags;
    int8_t qe1_sens;
    int8_t qe2_sens;
    uint8_t effector_mode; // called ps2_mode in original arcin firmware
    uint8_t ws2812b_mode;  // unused
};

#define ARCIN_CONFIG_FLAG_SEL_MULTI_TAP        (1 << 0)
#define ARCIN_CONFIG_FLAG_INVERT_QE1           (1 << 1)
#define ARCIN_CONFIG_FLAG_SWAP_8_9                (1 << 2)
#define ARCIN_CONFIG_FLAG_DIGITAL_TT_ENABLE    (1 << 3)
#define ARCIN_CONFIG_FLAG_RESERVED             (1 << 4)

typedef enum _effector_mode_option {
    START_E1_SEL_E2 = 0,
    START_E2_SEL_E1,
    START_E3_SEL_E4,
    START_E4_SEL_E3
} effector_mode_option, *peffector_mode_option;

#endif
