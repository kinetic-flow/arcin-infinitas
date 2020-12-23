#ifndef WS2812B_DEFINES_H
#define WS2812B_DEFINES_H

#include <stdint.h>
#include <os/time.h>
#include "fastled_shim.h"
#include "FastLED.h"
#include "color.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define WS2812B_DMA_BUFFER_LEN 26

#define WS2812B_MAX_LEDS 180
#define WS2812B_DEFAULT_LEDS 12

typedef uint32_t TProgmemRGBPalette16[16], *PPalette;

// duration of each frame, in milliseconds
//
// https://github.com/FastLED/FastLED/wiki/Interrupt-problems
// Each pixel takes 30 microseconds.
//  60 LEDs = 1800 us = 1.8ms
// 180 LEDs = 5400 us = 5.4ms
// So 20ms is more than enough to handle the worst case.

#define RGB_MANAGER_FRAME_MS 20

extern bool global_led_enable;

// Here, "0" is off, "1" refers to primary color, "2" is secondary, "3" is tertiary
typedef enum _WS2812B_Mode {
    // static   - all LEDs on 1
    // animated - this is the "breathe" effect (cycles between 0 and 1)
    // tt       - same as static (with default fade in/out only)
    WS2812B_MODE_SINGLE_COLOR,

    // same as single color, except with (1 and 2) instead of (0 and 1)
    WS2812B_MODE_TWO_COLOR_FADE,

    // static   - each LED takes 1/2/3
    // animated - each LED cycles through 1/2/3
    // tt       - controls animation speed and direction
    WS2812B_MODE_TRICOLOR,

    // static   - dots using 1/2/3
    // animated - same as static but rotates
    // tt       - controls animation speed and direction
    // mult     - controls the number of dots
    WS2812B_MODE_DOTS,

    // static   - all LEDs have the same color (somewhere on the hue spectrum)
    // animated - all LEDs cycle through hue spectrum
    // tt       - controls animation speed and direction
    WS2812B_MODE_STATIC_RAINBOW,

    // static   - each LED represents hue value on the palette
    // animated - static, but rotates through
    // tt       - controls animation speed and direction
    // mult     - controls the wave length
    WS2812B_MODE_RAINBOW_WAVE,

    // static   - picks a random color at boot and all LEDs use this
    // animated - n/a (same as static)
    // tt       - whenever activated, pick a random hue for all LEDs
    WS2812B_MODE_RANDOM_HUE,
} WS2812B_Mode;

typedef enum _WS2812B_Palette {
    WS2812B_PALETTE_RAINBOW,
    WS2812B_PALETTE_PARTY,
    WS2812B_PALETTE_HEAT,
    WS2812B_PALETTE_LAVA,
    WS2812B_PALETTE_OCEAN,
    WS2812B_PALETTE_FOREST,
} WS2812B_Palette;

// Same as FastLED RainbowColors_p but in reverse order
extern const TProgmemRGBPalette16 RainbowColors_reverse_p FL_PROGMEM =
{
   0xD5002B, 0xAB0055, 0x7F0081, 0x5500AB,
   0x2A00D5, 0x0000FF, 0x0056AA, 0x00AB55,
   0x00D52A, 0x00FF00, 0x56D500, 0xABAB00,
   0xAB7F00, 0xAB5500, 0xD52A00, 0xFF0000
};

void chsv_from_colorrgb(ColorRgb color, CHSV& chsv) {
    CRGB crgb(color.Red, color.Green, color.Blue);
    chsv = rgb2hsv_approximate(crgb);
}

class WS2812B {
    private:
        uint8_t dmabuf[WS2812B_DMA_BUFFER_LEN];
        volatile uint32_t cnt;
        volatile bool busy;
        uint8_t num_leds = WS2812B_MAX_LEDS;
        bool order_reversed = false;
        uint8_t brightness = UINT8_MAX;

        void schedule_dma() {
            cnt--;
            
            DMA1.reg.C[6].NDTR = WS2812B_DMA_BUFFER_LEN;
            DMA1.reg.C[6].MAR = (uint32_t)&dmabuf;
            DMA1.reg.C[6].PAR = (uint32_t)&TIM4.CCR3;
            DMA1.reg.C[6].CR =
                (0 << 10) | (1 << 8) | (1 << 7) | (0 << 6) | (1 << 4) | (1 << 1) | (1 << 0);
        }

        void set_color(CRGB rgb) {
            CRGB rgb_adjusted = rgb;
            rgb_adjusted.fadeToBlackBy(UINT8_MAX - this->brightness);
            this->set_color_raw(rgb_adjusted.red, rgb_adjusted.green, rgb_adjusted.blue);
        }
        
        void set_color_raw(uint8_t r, uint8_t g, uint8_t b) {
            uint32_t n = 0;
            
            dmabuf[0] = 0;
            n++;

            for(uint32_t i = 8; i-- > 0; n++) {
                dmabuf[n] = g & (1 << i) ? 58 : 29;
            }
            
            for(uint32_t i = 8; i-- > 0; n++) {
                dmabuf[n] = r & (1 << i) ? 58 : 29;
            }
            
            for(uint32_t i = 8; i-- > 0; n++) {
                dmabuf[n] = b & (1 << i) ? 58 : 29;
            }
            
            dmabuf[n] = 0;
        }
        
    public:
        CRGB leds[WS2812B_MAX_LEDS];

        void init(uint8_t num_leds, bool order_reversed) {
            this->busy = false;
            this->cnt = 0;

            // num_leds should be [1, MAX]
            this->num_leds = min(num_leds, WS2812B_MAX_LEDS);
            if (this->num_leds == 0) {
                this->num_leds = WS2812B_DEFAULT_LEDS;
            }
            this->order_reversed = order_reversed;

            RCC.enable(RCC.TIM4);
            RCC.enable(RCC.DMA1);
            
            Interrupt::enable(Interrupt::DMA1_Channel7);
            
            TIM4.ARR = (72000000 / 800000) - 1; // period = 90, 0 = 29, 1 = 58
            TIM4.CCR3 = 0;
            
            TIM4.CCMR2 = (6 << 4) | (1 << 3);
            TIM4.CCER = 1 << 8;
            TIM4.DIER = 1 << 8;
            
            GPIOB[8].set_af(2);
            GPIOB[8].set_mode(Pin::AF);
            GPIOB[8].set_pull(Pin::PullNone);

            TIM4.CR1 = 1 << 0;
            
            Time::sleep(1);
        }

        void show() {
            if (busy) {
                return;
            }
            busy = true;
            cnt = this->num_leds;
            if (order_reversed) {
                set_color(this->leds[cnt - 1]);
            } else {
                set_color(this->leds[0]);
            }
            schedule_dma();
        }

        void set_brightness(uint8_t brightness) {
            this->brightness = dim8_lin(brightness);
        }

        uint8_t get_num_leds() {
            return this->num_leds;
        }

        void irq() {
            DMA1.reg.C[6].CR = 0;
            DMA1.reg.IFCR = 1 << 24;
            
            if (cnt) {
                if (order_reversed) {
                    set_color(this->leds[this->cnt - 1]);
                } else {
                    set_color(this->leds[this->num_leds - this->cnt]);
                }
                schedule_dma();
            } else {
                busy = false;
            }
        }
};

class RGBManager {

    WS2812B ws2812b;

    uint32_t last_hid_report = 0;
    uint32_t last_outdated_hid_check = 0;

    // reacting to tt movement (stationary / moving)
    // any movement instantly increases it to -127 or +127
    // no movement - slowly reaches 0 over time
    int8_t tt_activity = 0;
    uint32_t last_tt_activity_time = 0;
    uint16_t tt_fade_out_time = 0;
    int8_t previous_tt = 0;

    // user-defined color mode
    WS2812B_Mode rgb_mode = WS2812B_MODE_SINGLE_COLOR;
    rgb_config_flags flags = {0};
    uint8_t multiplicity = 0;

    // user-defined modifiers
    uint8_t default_darkness = 0;
    uint8_t idle_brightness = 0;
    accum88 idle_animation_speed = 0;
    int16_t tt_animation_speed_10x = 0; // divide by 10 to get actual multiplier

    // user-defined colors
    CHSV hsv_primary;
    CHSV hsv_secondary;
    CHSV hsv_tertiary;

    // for random hue (from palette)
    uint8_t hue_temporary;
    bool ready_for_new_hue = false;

    // shift values that modify colors, ranges from [0 - UINT16_MAX]
    uint16_t shift_value = 0;
    uint32_t tt_time_travel_base_ms = 0;

    // for palette-based RGB modes
    CRGBPalette256 current_palette;

    private:    
        void update_static(CHSV& hsv) {
            fill_solid(ws2812b.leds, ws2812b.get_num_leds(), hsv);
            show();
        }

        void update_static(CRGB& rgb) {
            fill_solid(ws2812b.leds, ws2812b.get_num_leds(), rgb);
            show();
        }

        uint8_t calculate_brightness() {
            uint16_t brightness;
            if (flags.ReactToTt) {
                // start out with max brightness..
                brightness = UINT8_MAX;
                // and decrease with TT activity
                brightness -= scale8(
                    UINT8_MAX - idle_brightness,
                    quadwave8(127 + abs(tt_activity)));

            } else {
                // full brightness
                brightness = UINT8_MAX;
            }
            
            // finally, scale everything down by overall brightness override
            return scale8(brightness, UINT8_MAX - default_darkness);
        }

        void update(CHSV& hsv, uint8_t index) {
            ws2812b.leds[index] = hsv;
        }

        void show() {
            ws2812b.set_brightness(calculate_brightness());
            show_without_dimming();
        }

        void show_without_dimming() {
            ws2812b.show();
        }

        void set_off() {
            fill_solid(ws2812b.leds, ws2812b.get_num_leds(), CRGB::Black);
            show_without_dimming();
        }

        accum88 calculate_adjusted_speed(WS2812B_Mode rgb_mode, uint8_t raw_value) {
            // temporarily add precision
            uint32_t raw_value_1k = raw_value * 1000;
            accum88 adjusted;
            switch(rgb_mode) {
                case WS2812B_MODE_SINGLE_COLOR:
                case WS2812B_MODE_TWO_COLOR_FADE:
                    // BPM. Must match UI calculation
                    raw_value_1k = raw_value_1k * raw_value / UINT8_MAX;
                    break;

                case WS2812B_MODE_TRICOLOR:
                case WS2812B_MODE_DOTS:
                case WS2812B_MODE_RAINBOW_WAVE:
                    // RPM. Must match UI calculation
                    raw_value_1k = raw_value_1k / 2;
                    break;

                case WS2812B_MODE_STATIC_RAINBOW:                
                    // it's way too distracting otherwise
                    raw_value_1k = raw_value_1k / 8;
                    break;

                case WS2812B_MODE_RANDOM_HUE:
                default:
                    break;
            }

            adjusted = 0;
            adjusted |= (raw_value_1k / 1000) << 8;
            adjusted |= ((raw_value_1k % 1000) * 255 / 1000) & 0xFF;

            // 0 bpm is OK, but don't let it fall between 0-1 bpm since the library will convert
            // accum88 into uint8
            if (adjusted < 256) {
                adjusted = 0;
            }

            return adjusted;
        }

    public:
        void init(rgb_config* config) {
            // parse flags
            this->flags = config->Flags;
            this->tt_fade_out_time = 0;
            if (config->Flags.FadeOutFast) {
                this->tt_fade_out_time += 400;
            }
            if (config->Flags.FadeOutSlow) {
                this->tt_fade_out_time += 800;
            }

            chsv_from_colorrgb(config->RgbPrimary, this->hsv_primary);
            chsv_from_colorrgb(config->RgbSecondary, this->hsv_secondary);
            chsv_from_colorrgb(config->RgbTertiary, this->hsv_tertiary);

            this->default_darkness = config->Darkness;
            this->idle_brightness = config->IdleBrightness;

            this->idle_animation_speed =
                calculate_adjusted_speed((WS2812B_Mode)config->Mode, config->IdleAnimationSpeed);
            
            this->tt_animation_speed_10x = config->TtAnimationSpeed;

            set_mode(
                (WS2812B_Mode)config->Mode,
                (WS2812B_Palette)config->ColorPalette,
                config->Multiplicity);

            ws2812b.init(config->NumberOfLeds, config->Flags.FlipDirection);
            set_off();
        }

        void set_palette(WS2812B_Mode rgb_mode, WS2812B_Palette palette) {
            switch(palette) {
                case WS2812B_PALETTE_PARTY:
                    current_palette = PartyColors_p;
                    break;

                case WS2812B_PALETTE_RAINBOW:
                default:
                    if (rgb_mode == WS2812B_MODE_RAINBOW_WAVE){
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
        
        void set_mode(WS2812B_Mode rgb_mode, WS2812B_Palette palette, uint8_t multiplicity) {
            this->rgb_mode = rgb_mode;
            this->multiplicity = max(1, multiplicity);

            // seed random
            random16_add_entropy(serial_num() >> 16);
            random16_add_entropy(serial_num());
            hue_temporary = random8();

            // pre-initialize color palette
            switch(rgb_mode) {
                case WS2812B_MODE_TWO_COLOR_FADE:
                    current_palette = CRGBPalette256(hsv_primary, hsv_secondary);
                    break;

                case WS2812B_MODE_RANDOM_HUE:
                case WS2812B_MODE_STATIC_RAINBOW:
                case WS2812B_MODE_RAINBOW_WAVE:                
                    set_palette(rgb_mode, palette);
                    break;

                default:
                    break;
            }
        }
        
        void update_from_hid(ColorRgb color) {
            if (!global_led_enable || !flags.EnableHidControl) {
                return;
            }
            last_hid_report = Time::time();

            CHSV chsv;
            chsv_from_colorrgb(color, chsv);
            this->update_static(chsv);
        }

        void update_turntable_activity(uint32_t now, int8_t tt) {
            // Detect TT activity; framerate dependent, of course.
            switch (tt) {
                case 1:
                    tt_activity = 127;
                    last_tt_activity_time = now;
                    break;

                case -1:
                    tt_activity = -127;
                    last_tt_activity_time = now;
                    break;

                case 0:
                default:
                    if (last_tt_activity_time == 0) {
                        tt_activity = 0;
                    } else if (tt_activity != 0) {
                        uint16_t time_since_last_tt = now - last_tt_activity_time;
                        if (time_since_last_tt < tt_fade_out_time) {
                            uint16_t delta = tt_fade_out_time - time_since_last_tt;
                            int16_t temp = tt_activity;
                            if (temp > 0) {
                                temp = 
                                    ((int16_t)127) * delta / tt_fade_out_time;
                            } else {
                                temp = 
                                    ((int16_t)-127) * delta / tt_fade_out_time;
                            }

                            tt_activity = temp;

                        } else {
                            tt_activity = 0;
                        }
                    }
                    break;
            }

            // when turntable is CCW, pause idle animation by "stopping" time progression
            if (tt_activity != 0) {
                tt_time_travel_base_ms -= scale8(RGB_MANAGER_FRAME_MS, quadwave8(abs(tt_activity)));
            }
        }

        int16_t calculate_shift(int8_t tt, int8_t tt_multiplier) {
            const int16_t tt_animation = tt_animation_speed_10x * tt_multiplier;
            if (!flags.ReactToTt || tt_activity == 0 || tt_animation == 0) {
                // TT movement has no effect
                return 0;
            }

            return tt_animation * tt_activity / 127;            
        }

        void update_shift(int8_t tt, int8_t tt_multiplier) {
            shift_value += calculate_shift(tt, tt_multiplier);
        }

        uint8_t update_and_get_led_number_shift(int8_t tt, int8_t tt_multiplier) {
            int16_t delta = calculate_shift(tt, tt_multiplier);

            // possible range is [0... N*0x200) where N = number of LEDs
            const uint16_t maximum = ws2812b.get_num_leds() * (1 << 10);

            shift_value += delta;

            // did it roll under/over?
            if (maximum <= shift_value) {
                if (shift_value < maximum * 2) {
                    shift_value = shift_value - maximum;
                } else if ((UINT16_MAX - maximum * 2) < shift_value) {
                    shift_value = shift_value + maximum;
                } else {
                    // way off bounds
                    shift_value = 0;
                }
            }

            return (uint8_t)(shift_value >> 10);
        }

        // tt +1 is clockwise, -1 is counter-clockwise
        void update_colors(int8_t tt) {
            // prevent frequent updates - use 20ms as the framerate. This framerate will have
            // downstream effects on the various color algorithms below.
            uint32_t now = Time::time();
            if ((now - last_outdated_hid_check) < RGB_MANAGER_FRAME_MS) {
                return;
            }
            last_outdated_hid_check = now;

            // if there was a HID report recently, don't take over control
            if ((last_hid_report != 0) && ((now - last_hid_report) < 5000)) {
                return;
            }
            if (!global_led_enable) {
                this->set_off();
                return;
            }

            if (flags.ReactToTt){
                update_turntable_activity(now, tt);
            }

            switch(rgb_mode) {
                case WS2812B_MODE_STATIC_RAINBOW:
                {
                    uint8_t index = beat8(idle_animation_speed, tt_time_travel_base_ms);

                    // +20 seems good
                    update_shift(tt, 20);
                    index += shift_value >> 8;

                    CRGB color = ColorFromPalette(current_palette, index, UINT8_MAX, NOBLEND);
                    this->update_static(color);
                }
                break;

                case WS2812B_MODE_RAINBOW_WAVE:
                {
                    uint8_t step = 255 / (ws2812b.get_num_leds() * (multiplicity + 1) / 2);

                    // we actually want to go "backwards" so that each color seem to be rotating clockwise.
                    uint8_t start_index =
                        UINT8_MAX - beat8(idle_animation_speed, -tt_time_travel_base_ms);

                    // -60 seems good
                    update_shift(tt, -60);
                    start_index += shift_value >> 8;

                    fill_palette(
                        ws2812b.leds,
                        ws2812b.get_num_leds(),
                        start_index,
                        step,
                        current_palette,
                        UINT8_MAX,
                        NOBLEND
                        );
                    show();
                }
                break;

                case WS2812B_MODE_TRICOLOR:
                {
                    // -12 seems good
                    uint8_t pixel_shift = update_and_get_led_number_shift(tt, -12);
                    for (int led = 0; led < ws2812b.get_num_leds(); led++) {
                        CHSV* color = NULL;
                        switch ((led + pixel_shift) % ws2812b.get_num_leds() % 3) {
                            case 0:
                            default:
                                color = &hsv_primary;
                                break;
                            case 1:
                                color = &hsv_secondary;
                                break;
                            case 2:
                                color = &hsv_tertiary;
                                break;
                        }                        
                        this->update(*color, led);
                    }
                    this->show();
                }
                break;

                case WS2812B_MODE_TWO_COLOR_FADE:
                {
                    uint8_t progress;
                    if (this->flags.ReactToTt) {
                        // normally, all LEDs are primary color
                        // any turntable activity instantly changes to secondary color, then it
                        //     graudally fades back to the primary color
                        progress = quadwave8(abs(tt_activity));
                    } else {
                        progress = ease8InOutQuad(beatsin8(idle_animation_speed));
                    }

                    CRGB rgb = ColorFromPalette(current_palette, progress, UINT8_MAX, NOBLEND);
                    this->update_static(rgb);
                }
                break;

                case WS2812B_MODE_RANDOM_HUE:
                {
                    if (this->flags.ReactToTt) {
                        if (abs8(tt_activity) < 32) {
                            ready_for_new_hue = true;
                        }
                        if ((this->previous_tt == 0) && (tt != 0) && (ready_for_new_hue)) {
                            // TT triggered, time to pick a new hue value
                            // pick one that is not too similar to the previous one
                            hue_temporary = hue_temporary + random8(30, UINT8_MAX-30);
                            ready_for_new_hue = false;
                        }
                    }

                    CRGB rgb = ColorFromPalette(current_palette, hue_temporary, UINT8_MAX, NOBLEND);
                    this->update_static(rgb);
                }
                break;

                case WS2812B_MODE_DOTS:
                {
                    // 16 seems good
                    uint8_t dot1 = update_and_get_led_number_shift(tt, 16);
                    uint8_t dot2 = UINT8_MAX;
                    uint8_t dot3 = UINT8_MAX;
                    if (2 == multiplicity) {
                        dot2 = (dot1 + (ws2812b.get_num_leds() / 2)) % ws2812b.get_num_leds();
                    } else if (3 <= multiplicity) {
                        dot2 = (dot1 + (ws2812b.get_num_leds() / 3)) % ws2812b.get_num_leds();
                        dot3 = (dot1 + (ws2812b.get_num_leds() / 3) * 2) % ws2812b.get_num_leds();
                    }
                    CHSV hsv_off(0, 0, 0);
                    for (uint8_t led = 0; led < ws2812b.get_num_leds(); led++) {
                        if (led == dot1) {
                            this->update(hsv_primary, led);
                        } else if (led == dot2) {
                            this->update(hsv_secondary, led);
                        } else if (led == dot3) {
                            this->update(hsv_tertiary, led);
                        } else {
                            this->update(hsv_off, led);
                        }
                    }
                    this->show();
                }
                break;

                case WS2812B_MODE_SINGLE_COLOR:
                default:
                {
                    if (this->idle_animation_speed == 0 || this->flags.ReactToTt) {
                        // just use a solid color, and let the turntable dimming logic take care of
                        // fade in/out
                        this->update_static(hsv_primary);
                    } else {
                        uint8_t brightness = beatsin8(idle_animation_speed, 20);
                        CHSV hsv(hsv_primary.hue, hsv_primary.sat, brightness);
                        this->update_static(hsv);
                    }
                }
                break;
            }

            if (flags.ReactToTt){
                this->previous_tt = tt;
            }
        }

        void irq() {
            ws2812b.irq();
        }
};

#endif
