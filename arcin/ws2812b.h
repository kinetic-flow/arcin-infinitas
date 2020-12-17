#ifndef WS2812B_DEFINES_H
#define WS2812B_DEFINES_H

#include <stdint.h>
#include <os/time.h>
#include "color.h"

#define WS2812B_DMA_BUFFER_LEN 26 // was 25 in arcin
#define WS2812B_MAX_LEDS 60

extern bool global_led_enable;

typedef enum _WS2812B_Mode {
  WS2812B_MODE_STATIC,
  WS2812B_MODE_TT_DIRECTIONAL,
} WS2812B_Mode;

class WS2812B {
    private:
        uint8_t dmabuf[WS2812B_DMA_BUFFER_LEN];
        volatile uint32_t cnt;
        volatile bool busy;
        uint32_t last_hid_report = 0;
        uint32_t last_outdated_hid_check = 0;

        uint8_t default_darkness = 0;
        ColorRgb rgb_primary = {0};
        ColorRgb rgb_secondary = {0};
        ColorRgb rgb_tertiary = {0};
        WS2812B_Mode rgb_mode = WS2812B_MODE_STATIC;

        uint8_t apply_darkness(uint8_t color) {
            uint16_t new_color = color;
            new_color = new_color * (__UINT8_MAX__ - default_darkness) / __UINT8_MAX__;
            return (uint8_t)new_color;
        }

        void update(ColorRgb rgb) {
            if (busy) {
                return;
            }

            busy = true;

            set_color(
                apply_darkness(rgb.Red),
                apply_darkness(rgb.Green),
                apply_darkness(rgb.Blue));

            cnt = WS2812B_MAX_LEDS;

            schedule_dma();
        }

        void set_off() {
            ColorRgb off = {0};
            this->update(off);
        }

        void schedule_dma() {
            cnt--;
            
            DMA1.reg.C[6].NDTR = WS2812B_DMA_BUFFER_LEN;
            DMA1.reg.C[6].MAR = (uint32_t)&dmabuf;
            DMA1.reg.C[6].PAR = (uint32_t)&TIM4.CCR3;
            DMA1.reg.C[6].CR = (0 << 10) | (1 << 8) | (1 << 7) | (0 << 6) | (1 << 4) | (1 << 1) | (1 << 0);
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
        void init() {
            busy = false;
            cnt = 0;

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
            this->set_off();
        }

        void set_default_colors(ColorRgb primary, ColorRgb secondary, ColorRgb tertiary) {
            rgb_primary = primary;
            rgb_secondary = secondary;
            rgb_tertiary = tertiary;
        }

        void set_darkness(uint8_t darkness) {
            default_darkness = darkness;
        }
        
        void update_from_hid(ColorRgb rgb) {
            if (!global_led_enable) {
                return;
            }
            last_hid_report = Time::time();
            this->update(rgb);
        }

        void update_colors(int8_t turntable_direction) {
            // update at most x milliseconds
            uint32_t now = Time::time();
            if ((now - last_outdated_hid_check) < 4) {
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

            switch(rgb_mode) {
                case WS2812B_MODE_TT_DIRECTIONAL:
                    switch (turntable_direction) {
                        case -1:
                            this->update(rgb_secondary);
                            break;
                        case 1:
                            this->update(rgb_tertiary);
                            break;
                        default:
                            this->update(rgb_primary);
                            break;
                    }
                    break;

                case WS2812B_MODE_STATIC:
                default:
                    this->update(rgb_primary);
                    break;
            }
        }

        void set_mode(WS2812B_Mode rgb_mode) {
            this->rgb_mode = rgb_mode;
        }
        
        void irq() {
            DMA1.reg.C[6].CR = 0;
            DMA1.reg.IFCR = 1 << 24;
            
            if(cnt) {
                schedule_dma();
            } else {
                busy = false;
            }
        }
};

#endif
