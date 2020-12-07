#ifndef TTSENS_DEFINES_H
#define TTSENS_DEFINES_H

#include "config.h"

void tt_sensitivity_init(int8_t qe1_sens);
void set_hid_resistance(uint8_t resistance);
uint32_t apply_qe1_sens_post(uint32_t raw);

#endif
