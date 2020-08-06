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

typedef enum _effector_mode_option {
	E1_E2 = 0,
	E2_E1,
	E3_E4,
	E4_E3
} effector_mode_option, *peffector_mode_option;

#endif
