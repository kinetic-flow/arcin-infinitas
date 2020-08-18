#ifndef MULTIFUNC_DEFINES_H
#define MULTIFUNC_DEFINES_H

#include <stdint.h>
#include "config.h"

void e2_update(bool pressed);

uint16_t get_multitap_output();

bool is_multitap_window_closed();

#endif