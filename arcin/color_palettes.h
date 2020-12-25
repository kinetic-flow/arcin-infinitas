#ifndef COLOR_PALETTES_DEFINES_H
#define COLOR_PALETTES_DEFINES_H

#include "FastLED.h"
#include "colorutils.h"

typedef enum _WS2812B_Palette {
    WS2812B_PALETTE_RAINBOW,
    WS2812B_PALETTE_DREAM,
    WS2812B_PALETTE_HAPPYSKY,
    WS2812B_PALETTE_TROOPERS,
    WS2812B_PALETTE_EMPRESS,
    WS2812B_PALETTE_TRICORO,
    WS2812B_PALETTE_CANNON_BALLERS,
    WS2812B_PALETTE_ROOTAGE,
    WS2812B_PALETTE_HEROIC_VERSE,
    WS2812B_PALETTE_BISTROVER,
} WS2812B_Palette;

extern const TProgmemRGBPalette16 RainbowColors_reverse_p FL_PROGMEM;
extern const TProgmemRGBPalette16 CannonBallers_p FL_PROGMEM;
extern const TProgmemRGBPalette16 Tricoro_p FL_PROGMEM;
extern const TProgmemRGBPalette16 Bistrover_p FL_PROGMEM;
extern const TProgmemRGBPalette16 Rootage_p FL_PROGMEM;
extern const TProgmemRGBPalette16 Troopers_p FL_PROGMEM;
extern const TProgmemRGBPalette16 Empress_p FL_PROGMEM;

DECLARE_GRADIENT_PALETTE(HeroicVerse_gp);
DECLARE_GRADIENT_PALETTE(Empress_gp);

void fill_from_palette(CRGBPalette256& current_palette, WS2812B_Palette palette, bool reverse_rainbow);

#endif