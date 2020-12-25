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

extern const TProgmemRGBPalette16 Rootage_p FL_PROGMEM =
{
    CRGB::DarkOrange,
    CRGB::Gold,
    CRGB::Gold,
    CRGB::Gold,

    CRGB::Yellow,
    CRGB::Yellow,
    CRGB::LightYellow,
    CRGB::Yellow,

    CRGB::Yellow,
    CRGB::Gold,
    CRGB::Gold,
    CRGB::Gold,

    CRGB::DarkOrange,
    CRGB::DarkOrange,
    CRGB::DarkOrange,
    CRGB::DarkOrange,
};

extern const TProgmemRGBPalette16 Troopers_p FL_PROGMEM =
{
    CRGB::DarkGreen,
    CRGB::DarkGreen,
    CRGB::DarkOliveGreen,
    CRGB::DarkGreen,

    CRGB::Green,
    CRGB::ForestGreen,
    CRGB::OliveDrab,
    CRGB::Green,

    CRGB::DarkGreen,
    CRGB::DarkOliveGreen,
    CRGB::DarkOliveGreen,
    CRGB::Green,

    CRGB::DarkGreen,
    CRGB::Green,
    CRGB::DarkGreen,
    CRGB::ForestGreen
};

DEFINE_GRADIENT_PALETTE(Empress_gp) {
    0, 0x59, 0x1f, 0x28,
    124, 0xca, 0x02, 0x2b,
    230, 0xff, 0x00, 0xff,
    255, 0x59, 0x1f, 0x28,
};

void fill_from_palette(CRGBPalette256& current_palette, WS2812B_Palette palette, bool reverse_rainbow) {
    switch(palette) {
        case WS2812B_PALETTE_DREAM:
            current_palette = PartyColors_p;
            break;

        case WS2812B_PALETTE_CANNON_BALLERS:
            current_palette = CannonBallers_p;
            break;

        case WS2812B_PALETTE_HAPPYSKY:
            current_palette = CloudColors_p;
            break;

        case WS2812B_PALETTE_TROOPERS:
            current_palette = Troopers_p;
            break;

        case WS2812B_PALETTE_EMPRESS:
            current_palette = Empress_gp;
            break;

        case WS2812B_PALETTE_TRICORO:
            current_palette = Tricoro_p;
            break;

        case WS2812B_PALETTE_BISTROVER:
            current_palette = Bistrover_p;
            break;

        case WS2812B_PALETTE_HEROIC_VERSE:
            current_palette = HeroicVerse_gp;
            break;

        case WS2812B_PALETTE_ROOTAGE:
            current_palette = Rootage_p;
            break;

        case WS2812B_PALETTE_RAINBOW:
        default:
            if (reverse_rainbow){
                // This makes the rainbow go counter-clockwise
                // It "looks" more correct in wave (multiple circle) configuration than the
                // clockwise.
                current_palette = RainbowColors_reverse_p;
            } else {
                current_palette = RainbowColors_p;
            }
            
            break;
    }
}
