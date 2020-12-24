#ifndef WS2812B_DEFINES_H
#define WS2812B_DEFINES_H

#include <stdint.h>
#include <os/time.h>
#include "fastled_shim.h"
#include "FastLED.h"
#include "color.h"
#include "color_palettes.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(a,b) (((a) > (b)) ? (a) : (b))

// duration of each frame, in milliseconds
//
// https://github.com/FastLED/FastLED/wiki/Interrupt-problems
// Each pixel takes 30 microseconds.
//  60 LEDs = 1800 us = 1.8ms
// 180 LEDs = 5400 us = 5.4ms
// So 20ms is more than enough to handle the worst case.

#define RGB_MANAGER_FRAME_MS 20

#define WS2812B_DMA_BUFFER_LEN 26
#define WS2812B_MAX_LEDS 180
#define WS2812B_DEFAULT_LEDS 12

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

extern WS2812B ws2812b_global;

template<EOrder RGB_ORDER = RGB>
class ArcinController : public CPixelLEDController<RGB_ORDER> {
    public:
        virtual void init() { }

    protected:
        virtual void showPixels(PixelController<RGB_ORDER> & pixels) {
            uint8_t count = 0;
            while (pixels.has(1)) {
                uint8_t r = pixels.loadAndScale0();
                uint8_t g = pixels.loadAndScale1();
                uint8_t b = pixels.loadAndScale2();

                ws2812b_global.leds[count] = CRGB(r, g, b);
                count += 1;

                pixels.advanceData();
                pixels.stepDithering();
            }

            ws2812b_global.show();
        }
};

#endif
