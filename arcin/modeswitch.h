#ifndef MODESWITCH_DEFINES_H
#define MODESWITCH_DEFINES_H

#include <stdint.h>
#include "config.h"

config_flags initialize_mode_switch(config_flags flags);

config_flags process_mode_switch(uint16_t raw_input);

extern bool analog_tt_reverse_direction;

#endif