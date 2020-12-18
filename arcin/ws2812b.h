#ifndef WS2812B_DEFINES_H
#define WS2812B_DEFINES_H

#include <stdint.h>
#include <os/time.h>
#include "fastled_hsv2rgb.h"
#include "color.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define WS2812B_DMA_BUFFER_LEN 26 // was 25 in arcin
#define WS2812B_MAX_LEDS 60
extern bool global_led_enable;

typedef enum _WS2812B_Mode {
    WS2812B_MODE_STATIC,
    WS2812B_MODE_TRICOLOR,
    WS2812B_MODE_STATIC_RAINBOW,
    WS2812B_MODE_CIRCULAR_RAINBOW,
} WS2812B_Mode;

void crgb_from_colorrgb(ColorRgb color, CRGB& crgb) {
    crgb.red = color.Red;
    crgb.green = color.Green;
    crgb.blue = color.Blue;
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

class RGBManager {

    WS2812B ws2812b;

    uint32_t last_hid_report = 0;
    uint32_t last_outdated_hid_check = 0;

    WS2812B_Mode rgb_mode = WS2812B_MODE_STATIC;
    rgb_config_flags flags = {0};

    uint8_t default_darkness = 0;
    int8_t speed = 0;

    CRGB rgb_primary;
    CRGB rgb_secondary;
    CRGB rgb_tertiary;

    uint16_t shift_value = 0;

    private:    
        uint8_t apply_darkness(uint8_t color) {
            uint16_t new_color = color;
            new_color = new_color * (UINT8_MAX - default_darkness) / UINT8_MAX;
            return (uint8_t)new_color;
        }

        void apply_darkness(CRGB& color) {
            color.red = apply_darkness(color.red);
            color.green = apply_darkness(color.green);
            color.blue = apply_darkness(color.blue);
        }

        void update_static(CHSV& hsv) {
            CRGB rgb(hsv);
            this->update_static(rgb);
        }

        void update_static(CRGB& rgb) {
            for (uint8_t i = 0; i < ws2812b.get_num_leds(); i++) {
                this->update(rgb, i);
            }

            this->update_complete();
        }

        void update(CHSV& hsv, uint8_t index) {
            CRGB rgb(hsv);
            this->update(rgb, index);
        }

        void update(CRGB& rgb, uint8_t index) {
            CRGB rgb_adjusted = rgb;
            apply_darkness(rgb_adjusted);
            ws2812b.update_led_color(rgb_adjusted, index);
        }

        void update_complete() {
            ws2812b.update_complete();
        }

        void set_off() {
            CRGB off;
            off.red = 0;
            off.green = 0;
            off.blue = 0;
            this->update_static(off);
        }

    public:
        void init(rgb_config_flags flags, uint8_t num_leds) {
            this->flags = flags;
            ws2812b.init(num_leds, flags.FlipDirection);
            this->set_off();
        }
        
        void set_mode(WS2812B_Mode rgb_mode) {
            this->rgb_mode = rgb_mode;
        }
        
        void set_default_colors(ColorRgb primary, ColorRgb secondary, ColorRgb tertiary) {
            crgb_from_colorrgb(primary, this->rgb_primary);
            crgb_from_colorrgb(secondary, this->rgb_secondary);
            crgb_from_colorrgb(tertiary, this->rgb_tertiary);
        }

        void set_darkness(uint8_t darkness) {
            this->default_darkness = darkness;
        }

        void set_speed(int8_t speed) {
            this->speed = speed;
        }
        
        void update_from_hid(ColorRgb color) {
            if (!global_led_enable || !flags.EnableHidControl) {
                return;
            }
            last_hid_report = Time::time();

            CRGB crgb;
            crgb_from_colorrgb(color, crgb);
            this->update_static(crgb);
        }

        void update_colors(int8_t tt) {
            // prevent frequent updates - use 20ms as the framerate. This framerate will have
            // downstream effects on the various color algorithms below.
            uint32_t now = Time::time();
            if ((now - last_outdated_hid_check) < 20) {
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

            // configurable speed is signed; convert this to unsigned
            // 0 should be the reasonable default speed.
            // (slowest / no movement being 0, highest being 255)
            uint8_t speed = this->speed - INT8_MIN;

            switch(rgb_mode) {
                // Static rainbow mode.
                //
                // All LEDs have the same color, but the hue can change.
                // When TT reactive is on, the turntable adjusts the hue.
                // In normal mode, hue shifts in one direction.
                //
                // Flip affects direction for both.
                // Speed affects how fast the hue changes.
                case WS2812B_MODE_STATIC_RAINBOW:
                {
                    int32_t tick = speed;
                    if (flags.FlipDirection) {
                        tick *= -1;
                    }

                    if (flags.ReactToTt) {
                        shift_value += tt * tick;
                    } else {
                        shift_value += tick;
                    }

                    CHSV hsv(shift_value >> 8, 255, 255);
                    this->update_static(hsv);
                }
                break;

                // Circular rainbow mode.
                //
                // Each LED covers a portion of the hue spectrum, coming full circle.
                // The math depends on the number of LEDs being accurate.
                //
                // TT reactive mode causes a hue shift.
                // Normal mode causes hue to shift over time.
                //
                // Flip reverse the entire LED strip direction.
                // Speed affects how fast the hue shift happens.
                case WS2812B_MODE_CIRCULAR_RAINBOW:
                {
                    int32_t tick = speed * 2;
                    if (flags.ReactToTt) {
                        shift_value += tt * tick;
                    } else {
                        shift_value += tick;
                    }

                    for (uint8_t led = 0; led < ws2812b.get_num_leds(); led++) {
                        uint16_t hue = 255 * led / ws2812b.get_num_leds();
                        hue = (hue + (shift_value >> 6)) % 255;
                        CHSV hsv(hue, 255, 255);
                        this->update(hsv, led);
                    }
                    this->update_complete();
                }
                break;

                // Tricolor mode.
                //
                // Each LED uses one of three colors, in order.
                case WS2812B_MODE_TRICOLOR:
                {
                    int32_t tick = speed;
                    if (flags.ReactToTt) {
                        shift_value += tt * tick;
                    } else {
                        shift_value += tick;
                    }

                    uint8_t pixel_shift = (shift_value >> 10) % 3;
                    for (int led = 0; led < ws2812b.get_num_leds(); led++) {
                        CRGB* rgb = NULL;
                        switch ((led + pixel_shift) % 3) {
                            case 0:
                            default:
                                rgb = &rgb_primary;
                                break;
                            case 1:
                                rgb = &rgb_secondary;
                                break;
                            case 2:
                                rgb = &rgb_tertiary;
                                break;
                        }                        
                        this->update(*rgb, led);
                    }
                    this->update_complete();
                }
                break;

                // Static color mode.
                //
                // Same color for all LEDs.
                // TT reactive mode causes all LEDs to use the secondary or the primary color
                // Flip only reverses the TT direction.
                case WS2812B_MODE_STATIC:
                default:
                {
                    switch (tt) {
                        case -1:
                            this->update_static(rgb_secondary);
                            break;
                        case 1:
                            this->update_static(rgb_tertiary);
                            break;
                        case 0:
                        default:
                            this->update_static(rgb_primary);
                            break;
                    }
                }
                break;
            }
        }

        void irq() {
            ws2812b.irq();
        }
};

#endif
