#ifndef WS2812B_DEFINES_H
#define WS2812B_DEFINES_H

#include <stdint.h>
#include <os/time.h>
#include "fastled.h"
#include "color.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define WS2812B_DMA_BUFFER_LEN 26

// https://github.com/FastLED/FastLED/wiki/Interrupt-problems
// Each pixel takes 30 microseconds.
//
// 1000hz mode => need to provide input every 1ms
// 250hz mode => need to provide input every 4ms
//
// 60 LEDs = 1800 us = 1.8ms
// 30 LEDs = 900 us  = 0.900ms
// 24 LEDs = 720 us  = 0.729ms
// 10 LEDs = 300 us  = 0.300ms
#define WS2812B_MAX_LEDS 30

extern bool global_led_enable;

// Here, "0" is off, "1" refers to primary color, "2" is secondary, "3" is tertiary
typedef enum _WS2812B_Mode {
    // static   - all LEDs on 1
    // animated - this is the "breathe" effect (cycles between 0 and 1)
    // tt       - same as static (with default fade in/out only)
    WS2812B_MODE_SINGLE_COLOR,

    // static   - each LED takes 1/2/3
    // animated - each LED cycles through 1/2/3
    // tt       - controls animation speed and direction
    WS2812B_MODE_TRICOLOR,

    // static   - all LEDs have the same color (somewhere on the hue spectrum)
    // animated - all LEDs cycle through hue spectrum
    // tt       - controls animation speed and direction
    WS2812B_MODE_STATIC_RAINBOW,

    // static   - each LED represents hue value on the rainbow spectrum
    // animated - static, but rotates through
    // tt       - controls animation speed and direction
    WS2812B_MODE_RAINBOW_SPIRAL,

    // static   - same as rainbow spiral except hue specturm spans three circles
    // animated - static, but rotates through
    // tt       - controls animation speed and direction
    WS2812B_MODE_RAINBOW_WAVE,

    // same as single color, except with (1 and 2) instead of (0 and 1)
    WS2812B_MODE_TWO_COLOR_FADE,

    // static   - todo
    // animated - todo
    // tt       - whenever activated, pick a random hue for all LEDs
    WS2812B_MODE_RANDOM_HUE,
} WS2812B_Mode;

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
        CRGB colors[WS2812B_MAX_LEDS];

        void schedule_dma() {
            cnt--;
            
            DMA1.reg.C[6].NDTR = WS2812B_DMA_BUFFER_LEN;
            DMA1.reg.C[6].MAR = (uint32_t)&dmabuf;
            DMA1.reg.C[6].PAR = (uint32_t)&TIM4.CCR3;
            DMA1.reg.C[6].CR = (0 << 10) | (1 << 8) | (1 << 7) | (0 << 6) | (1 << 4) | (1 << 1) | (1 << 0);
        }

        void set_color(CRGB rgb) {
            this->set_color(rgb.red, rgb.green, rgb.blue);
        }
        
        void set_color(uint8_t r, uint8_t g, uint8_t b) {
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
        void init(uint8_t num_leds, bool order_reversed) {
            this->busy = false;
            this->cnt = 0;

            // num_leds should be [1, MAX]
            // use a sensible unconfigured value (turn 0 into max)
            this->num_leds = min(num_leds, WS2812B_MAX_LEDS);
            if (this->num_leds == 0) {
                this->num_leds = WS2812B_MAX_LEDS;
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

        void update_led_color(CRGB rgb, uint8_t index) {
            if (busy) {
                return;
            }

            if (index >= this->num_leds) {
                return;
            }

            if (this->order_reversed) {
                colors[this->num_leds - index] = rgb;
            } else {
                colors[index] = rgb;
            }
        }

        void update_complete() {
            if (busy) {
                return;
            }
            busy = true;
            cnt = this->num_leds;
            set_color(this->colors[0]);
            schedule_dma();
        }

        uint8_t get_num_leds() {
            return this->num_leds;
        }

        void irq() {
            DMA1.reg.C[6].CR = 0;
            DMA1.reg.IFCR = 1 << 24;
            
            if (cnt) {
                set_color(this->colors[this->num_leds - this->cnt]);
                schedule_dma();
            } else {
                busy = false;
            }
        }
};

// duration of each frame, in milliseconds
#define RGB_MANAGER_FRAME_MS 20

class RGBManager {

    WS2812B ws2812b;

    uint32_t last_hid_report = 0;
    uint32_t last_outdated_hid_check = 0;

    // reacting to tt movement (stationary / moving)
    // any movement instantly increases it to UINT8_MAX
    // no movement slowly decreases this value over time
    uint8_t tt_activity = 0;
    uint32_t last_tt_activity_time = 0;
    uint16_t tt_fade_out_time = 0;
    int8_t previous_tt = 0;

    // user-defined color mode
    WS2812B_Mode rgb_mode = WS2812B_MODE_SINGLE_COLOR;
    rgb_config_flags flags = {0};

    // user-defined modifiers
    uint8_t default_darkness = 0;
    uint8_t idle_brightness = 0;
    uint8_t idle_animation_speed = 0;
    int8_t tt_animation_speed = 0;

    // user-defined colors
    CHSV hsv_primary;
    CHSV hsv_secondary;
    CHSV hsv_tertiary;

    // for random hue
    uint8_t hue_temporary;
    bool ready_for_new_hue = false;

    // shift values that modify lights
    uint16_t shift_value = 0;
    int8_t shift_direction = 1;

    private:    
        void update_static(CHSV& hsv) {
            for (uint8_t i = 0; i < ws2812b.get_num_leds(); i++) {
                this->update(hsv, i);
            }

            this->update_complete();
        }

        uint8_t calculate_brightness() {
            uint16_t brightness;
            if (flags.ReactToTt) {
                // start out with idle brightness..
                brightness = idle_brightness;
                // and increase with TT activity
                brightness += scale8(UINT8_MAX - idle_brightness, tt_activity);
            } else {
                // full brightness
                brightness = UINT8_MAX;
            }
            
            // finally, apply overall darkness override
            return scale8(brightness, UINT8_MAX - default_darkness);
        }

        void update(CHSV& hsv, uint8_t index) {
            CHSV hsv_adjusted = hsv;
            hsv_adjusted.value = scale8(hsv_adjusted.value, calculate_brightness());
            CRGB rgb(hsv_adjusted);
            ws2812b.update_led_color(rgb, index);
        }

        void update_complete() {
            ws2812b.update_complete();
        }

        void set_off() {
            CHSV off(0, 0, 0);
            this->update_static(off);
        }

    public:
        void init(rgb_config_flags flags, uint8_t num_leds) {
            this->flags = flags;

            this->tt_fade_out_time = 0;
            if (flags.FadeOutFast) {
                tt_fade_out_time += 200;
            }
            if (flags.FadeOutSlow) {
                tt_fade_out_time += 400;
            }

            ws2812b.init(num_leds, flags.FlipDirection);
            this->set_off();

            // init for color modes
            hue_temporary = random8();
        }
        
        void set_mode(WS2812B_Mode rgb_mode) {
            this->rgb_mode = rgb_mode;
        }
        
        void set_default_colors(ColorRgb primary, ColorRgb secondary, ColorRgb tertiary) {
            chsv_from_colorrgb(primary, this->hsv_primary);
            chsv_from_colorrgb(secondary, this->hsv_secondary);
            chsv_from_colorrgb(tertiary, this->hsv_tertiary);
        }

        void set_darkness(uint8_t darkness) {
            this->default_darkness = darkness;
        }

        void set_idle_brightness(uint8_t idle_brightness) {
            this->idle_brightness = idle_brightness;
        }

        void set_animation_speed(int8_t idle_speed, int8_t tt_speed) {
            this->idle_animation_speed = idle_speed;
            this->tt_animation_speed = tt_speed;
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
                case -1:
                    tt_activity = UINT8_MAX;
                    last_tt_activity_time = now;
                    break;

                case 0:
                default:
                    if (last_tt_activity_time == 0) {
                        tt_activity = 0;
                    } else {
                        uint32_t delta = now - last_tt_activity_time;
                        if (delta < this->tt_fade_out_time) {
                            tt_activity = 
                                UINT8_MAX * (this->tt_fade_out_time - delta) / this->tt_fade_out_time;

                            tt_activity = quadwave8(tt_activity / 2);
                        } else {
                            tt_activity = 0;
                        }
                    }
                    break;
            }

            if (tt_activity < 64) {
                ready_for_new_hue = true;
            }
        }

        void update_shift(int8_t tt, int8_t idle_multiplier, int8_t tt_multiplier) {
            uint32_t idle_animation = idle_animation_speed * idle_multiplier;
            uint32_t tt_animation = tt * tt_animation_speed * tt_multiplier;

            // if TT movement has no effect, only idle animation is used
            if (!flags.ReactToTt || tt == 0 || tt_animation == 0) {
                shift_value += idle_animation;
                return;
            }

            shift_value += tt_animation;
            
            // if tt animation is moving in the same direction, use both
            if ((idle_animation > 0 && tt_animation > 0) ||
                (idle_animation < 0 && tt_animation < 0)) {
                shift_value += idle_animation;
                return;
            }

            // in all other cases:
            //  - idle animation is stationary
            //  - tt animatino in the opposite direction of idle animation
            //... just use the tt animation.
            return;
        }

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
                    update_shift(tt, 2, -4);

                    CHSV hsv(shift_value >> 8, 255, 255);
                    this->update_static(hsv);
                }
                break;

                case WS2812B_MODE_RAINBOW_SPIRAL:
                case WS2812B_MODE_RAINBOW_WAVE:
                {
                    update_shift(tt, -3, 5);

                    uint16_t number_of_circles = 1;
                    if (rgb_mode == WS2812B_MODE_RAINBOW_WAVE) {
                        number_of_circles = 3;
                    }

                    for (uint8_t led = 0; led < ws2812b.get_num_leds(); led++) {
                        uint16_t hue = 255 * led / (ws2812b.get_num_leds() * number_of_circles);
                        hue = (hue + (shift_value >> 6)) % 255;
                        if (rgb_mode == WS2812B_MODE_RAINBOW_WAVE) {
                            hue = UINT8_MAX - hue;
                        }
                        CHSV hsv(hue, 255, 255);
                        this->update(hsv, led);
                    }
                    this->update_complete();
                }
                break;

                case WS2812B_MODE_TRICOLOR:
                {
                    update_shift(tt, -2, 4);

                    uint8_t pixel_shift = (shift_value >> 10) % 3;
                    for (int led = 0; led < ws2812b.get_num_leds(); led++) {
                        CHSV* color = NULL;
                        switch ((led + pixel_shift) % 3) {
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
                    this->update_complete();
                }
                break;

                case WS2812B_MODE_TWO_COLOR_FADE:
                {
                    uint16_t progress = 0;
                    if (this->flags.ReactToTt) {
                        progress = tt_activity;
                    } else {
                        update_shift(0, 4, 0);
                        progress = quadwave8(shift_value >> 8);
                    }

                    // progress 0 => initial color
                    // progress 1-254 => something in between
                    // progress 255 => goal color

                    // on the hue spectrum, always travel in the direction that is shorter
                    int16_t h = (hsv_secondary.h - hsv_primary.h);
                    if (h > INT8_MAX) {
                        h = h - UINT8_MAX;
                    } else if (h < INT8_MIN) {
                        h = h + UINT8_MAX;
                    }
                    h = scale8(h, progress);
                    int8_t s = scale8(hsv_secondary.s - hsv_primary.s, progress);
                    int8_t v = scale8(hsv_secondary.v - hsv_primary.v, progress);
                    CHSV hsv(hsv_primary.h + h, hsv_primary.s + s, hsv_primary.v + v);
                    this->update_static(hsv);
                }
                break;

                case WS2812B_MODE_RANDOM_HUE:
                {
                    if (this->flags.ReactToTt) {
                        if ((this->previous_tt == 0) && (tt != 0) && (ready_for_new_hue)) {
                            // TT triggered, time to pick a new hue value
                            // pick one that is not too similar to the previous one
                            hue_temporary = hue_temporary + random8(255-60) + 30;
                            ready_for_new_hue = false;
                        }
                    }

                    CHSV hsv(hue_temporary, 255, 255);
                    this->update_static(hsv);
                }
                break;

                case WS2812B_MODE_SINGLE_COLOR:
                default:
                {
                    if (this->idle_animation_speed == 0 || this->flags.ReactToTt) {
                        this->update_static(hsv_primary);
                    } else {
                        // breathe mode
                        update_shift(0, 4, 0);
                        uint8_t brightness = quadwave8(shift_value >> 8);
                        // make 20 the "floor" since most LEDs have a hard time with really dark values
                        brightness = scale8(brightness, 255 - 20) + 20;
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
