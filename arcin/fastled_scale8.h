#ifndef SCALE8_H
#define SCALE8_H

#include <stdint.h>

// scale8.h copied from the FastLED project
// https://github.com/FastLED/FastLED
// Copyright (c) 2013 FastLED under the MIT License

inline uint8_t scale8(uint8_t i, uint8_t scale) {
	return (((uint16_t)i) * (1+(uint16_t)(scale))) >> 8;
}

inline uint8_t scale8_video_LEAVING_R1_DIRTY( uint8_t i, uint8_t scale) {
	uint8_t j = (((int)i * (int)scale) >> 8) + ((i&&scale)?1:0);
	return j;
}

inline uint8_t scale8_LEAVING_R1_DIRTY( uint8_t i, uint8_t scale) {
	return (((uint16_t)i) * ((uint16_t)(scale)+1)) >> 8;
}

#endif
