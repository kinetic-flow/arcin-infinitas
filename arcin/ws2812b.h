#ifndef WS2812B_DEFINES_H
#define WS2812B_DEFINES_H

#include <stdint.h>
#include <os/time.h>
#include "color.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define WS2812B_DMA_BUFFER_LEN 26 // was 25 in arcin
#define WS2812B_MAX_LEDS 60
extern bool global_led_enable;

typedef enum _WS2812B_Mode {
    WS2812B_MODE_STATIC,
    WS2812B_MODE_TRICOLOR,
    WS2812B_MODE_RAINBOW,
} WS2812B_Mode;

class WS2812B {
    private:
        uint8_t dmabuf[WS2812B_DMA_BUFFER_LEN];
        volatile uint32_t cnt;
        volatile bool busy;
        uint8_t num_leds = WS2812B_MAX_LEDS;
        ColorRgb colors[WS2812B_MAX_LEDS];

        void schedule_dma() {
            cnt--;
            
            DMA1.reg.C[6].NDTR = WS2812B_DMA_BUFFER_LEN;
            DMA1.reg.C[6].MAR = (uint32_t)&dmabuf;
            DMA1.reg.C[6].PAR = (uint32_t)&TIM4.CCR3;
            DMA1.reg.C[6].CR = (0 << 10) | (1 << 8) | (1 << 7) | (0 << 6) | (1 << 4) | (1 << 1) | (1 << 0);
        }

        void set_color(ColorRgb rgb) {
            this->set_color(rgb.Red, rgb.Green, rgb.Blue);
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
        void init(uint8_t num_leds) {
            this->busy = false;
            this->cnt = 0;

            // num_leds should be [1, MAX]
            // use a sensible unconfigured value (turn 0 into max)
            this->num_leds = min(num_leds, WS2812B_MAX_LEDS);
            if (this->num_leds == 0) {
                this->num_leds = WS2812B_MAX_LEDS;
            }

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

        void update_led_color(ColorRgb rgb, uint8_t index) {
            if (busy) {
                return;
            }

            if (index < this->num_leds) {
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
    ColorRgb rgb_primary = {0};
    ColorRgb rgb_secondary = {0};
    ColorRgb rgb_tertiary = {0};

    uint8_t tt_shift = 0;

    private:    
        uint8_t apply_darkness(uint8_t color) {
            uint16_t new_color = color;
            new_color = new_color * (__UINT8_MAX__ - default_darkness) / __UINT8_MAX__;
            return (uint8_t)new_color;
        }

        void apply_darkness(ColorRgb* color) {
            color->Red = apply_darkness(color->Red);
            color->Green = apply_darkness(color->Green);
            color->Blue = apply_darkness(color->Blue);
        }

        void update_static(ColorRgb rgb) {
            for (uint8_t i = 0; i < ws2812b.get_num_leds(); i++) {
                this->update(rgb, i);
            }

            this->update_complete();
        }

        void update(ColorRgb rgb, uint8_t index) {
            apply_darkness(&rgb);
            ws2812b.update_led_color(rgb, index);
        }

        void update_complete() {
            ws2812b.update_complete();
        }

        void set_off() {
            ColorRgb off = {0};
            this->update_static(off);
        }

    public:
        void init(rgb_config_flags flags, uint8_t num_leds) {
            this->flags = flags;
            ws2812b.init(num_leds);
            this->set_off();
        }
        
        void set_mode(WS2812B_Mode rgb_mode) {
            this->rgb_mode = rgb_mode;
        }
        
        void set_default_colors(ColorRgb primary, ColorRgb secondary, ColorRgb tertiary) {
            this->rgb_primary = primary;
            this->rgb_secondary = secondary;
            this->rgb_tertiary = tertiary;
        }

        void set_darkness(uint8_t darkness) {
            this->default_darkness = darkness;
        }
        
        void update_from_hid(ColorRgb rgb) {
            if (!global_led_enable || !flags.EnableHidControl) {
                return;
            }
            last_hid_report = Time::time();
            this->update_static(rgb);
        }

        void update_colors(int8_t raw_turntable_direction) {
            // prevent frequent updates
            uint32_t now = Time::time();
            if ((now - last_outdated_hid_check) < 10) {
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

            int8_t tt = raw_turntable_direction;
            if (flags.FlipDirection) {
                tt *= -1;
            }

            switch(rgb_mode) {
                case WS2812B_MODE_TRICOLOR:
                    {
                        uint8_t shift = 0;
                        if (flags.ReactToTt) {
                            tt_shift += tt;
                            shift = (tt_shift >> 4) % 3;
                        }

                        for (int led = 0; led < ws2812b.get_num_leds(); led++) {
                            ColorRgb* rgb = NULL;
                            switch ((led + shift) % 3) {
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

                case WS2812B_MODE_STATIC:
                default:
                    if (flags.ReactToTt) {
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
                    } else {
                        this->update_static(rgb_primary);
                    }

                    break;
            }
        }

        void irq() {
            ws2812b.irq();
        }
};

#endif
