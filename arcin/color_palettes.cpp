#include "FastLED.h"
#include "colorutils.h"
#include "color_palettes.h"

// Same as FastLED RainbowColors_p but in reverse order
extern const TProgmemRGBPalette16 RainbowColors_reverse_p FL_PROGMEM =
{
   0xD5002B, 0xAB0055, 0x7F0081, 0x5500AB,
   0x2A00D5, 0x0000FF, 0x0056AA, 0x00AB55,
   0x00D52A, 0x00FF00, 0x56D500, 0xABAB00,
   0xAB7F00, 0xAB5500, 0xD52A00, 0xFF0000
};

DEFINE_GRADIENT_PALETTE( HappySky_gp ) {
    0, 0xff, 0xff, 0xff,
    25, 0xe6, 0xef, 0xf5,
    58, 0x4a, 0xbe, 0xff,
    102, 0x20, 0x69, 0xad,
    178, 0x4a, 0xbe, 0xff,
    229, 0xff, 0xff, 0xff,
    255, 0xff, 0xff, 0xff
};

extern const TProgmemRGBPalette16 Lincle_p FL_PROGMEM =
{
    CRGB::White,
    CRGB::OrangeRed,
    CRGB::OrangeRed,
    CRGB::OrangeRed,

    CRGB::OrangeRed,
    CRGB::OrangeRed,
    CRGB::OrangeRed,
    CRGB::OrangeRed,

    CRGB::OrangeRed,
    CRGB::OrangeRed,
    CRGB::SkyBlue,
    CRGB::SkyBlue,

    CRGB::SkyBlue,
    CRGB::SkyBlue,
    CRGB::SkyBlue,
    CRGB::White,
};