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

extern const TProgmemRGBPalette16 CannonBallers_p FL_PROGMEM =
{
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,

    CRGB::Red,
    CRGB::Red,
    CRGB::DimGray,
    CRGB::DimGray,

    CRGB::Green,
    CRGB::Green,
    CRGB::Green,
    CRGB::Green,

    CRGB::Green,
    CRGB::Green,
    CRGB::DimGray,
    CRGB::DimGray,
};

extern const TProgmemRGBPalette16 Tricoro_p FL_PROGMEM =
{
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,

    CRGB::Yellow,
    CRGB::Yellow,
    CRGB::Yellow,
    CRGB::Yellow,
    CRGB::Gray,

    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Gray,
};

extern const TProgmemRGBPalette16 Bistrover_p FL_PROGMEM =
{
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Yellow,

    CRGB::Yellow,
    CRGB::Yellow,
    CRGB::Blue,
    CRGB::Blue,

    CRGB::Blue,
    CRGB::Blue,
    CRGB::Yellow,
    CRGB::Yellow,

    CRGB::Yellow,
    CRGB::Green,
    CRGB::Green,
    CRGB::Blue,
};

DEFINE_GRADIENT_PALETTE(HeroicVerse_gp) {
    0, 0x80, 0x00, 0x80,
    43, 0x4f, 0x2b, 0x9a,
    186, 0xda, 0x07, 0xda,
    255, 0x80, 0x00, 0x80
};
