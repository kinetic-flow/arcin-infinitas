#include <stdint.h>

#ifndef COLOR_DEFINES_H
#define COLOR_DEFINES_H

typedef struct _ColorRgb {
    uint8_t Red;
    uint8_t Green;
    uint8_t Blue;
} ColorRgb, *PColorRgb;

static_assert(sizeof(ColorRgb) == (sizeof(uint8_t) * 3), "size mismatch");

#endif
