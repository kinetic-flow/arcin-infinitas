#include <rcc/rcc.h>
#include <gpio/gpio.h>
#include <interrupt/interrupt.h>
#include <timer/timer.h>
#include <dma/dma.h>
#include <os/time.h>
#include <usb/usb.h>
#include <usb/descriptor.h>

#include "report_desc.h"
#include "usb_strings.h"
#include "configloader.h"
#include "config.h"

#include "inf_defines.h"
#include "remap.h"
#include "multifunc.h"
#include "debounce.h"
#include "modeswitch.h"
#include "analog_button.h"
#include "rgbmanager.h"

#define DEBUG_TIMING_GAMEPAD 0

#define ARRAY_SIZE(x) \
    ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define max(a,b) (((a) > (b)) ? (a) : (b))

static const uint16_t infinitas_keys[] = {
    INFINITAS_BUTTON_1,
    INFINITAS_BUTTON_2,
    INFINITAS_BUTTON_3,
    INFINITAS_BUTTON_4,
    INFINITAS_BUTTON_5,
    INFINITAS_BUTTON_6,
    INFINITAS_BUTTON_7,
    INFINITAS_BUTTON_E1,
    INFINITAS_BUTTON_E2,
    INFINITAS_BUTTON_E3,
    INFINITAS_BUTTON_E4

    // [11] = digital tt CW
    // [12] = digital tt CCW
};

static uint32_t& reset_reason = *(uint32_t*)0x10000000;

static bool do_reset_bootloader;
static bool do_reset;

void reset() {
    SCB.AIRCR = (0x5fa << 16) | (1 << 2); // SYSRESETREQ
}

void reset_bootloader() {
    reset_reason = 0xb007;
    reset();
}

Configloader configloader(0x801f800);

config_t config;

/* 
 // origial hardware ID for arcin - expected by firmware flash
 // and the settings tool
auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1d50, 0x6080, 0x110, 1, 2, 3, 1);
*/

// Hardware ID Infinitas controller: 0x1ccf, 0x8048
// The game detects this and automatically uses its own (internal) key config
// overridng any user settings in the launcher
auto dev_desc = device_desc(
    0x200,  // bcdUSB
    0,      // bdeviceClass
    0,      // bDeviceSubClass
    0,      // bDeviceProtocol
    64,     // bMaxPacketSize0
    0x1CCF, // idVendor
    0x8048, // idProduct
    0x110,  // bcdDevice
    STRING_ID_Manufacturer,
    STRING_ID_Product,
    STRING_ID_Serial,
    1);     // bNumConfigurations

auto conf_desc_1000hz = configuration_desc(2, 1, 0, 0xc0, 0,
    // HID interface.
    interface_desc(0, 0, 1, 0x03, 0x00, 0x00, 0,
        hid_desc(0x111, 0, 1, 0x22, sizeof(report_desc)),
        endpoint_desc(0x81, 0x03, 16, 1)
    ),
    interface_desc(1, 0, 1, 0x03, 0x00, 0x00, 0,
        hid_desc(0x111, 0, 1, 0x22, sizeof(keyb_report_desc)),
        endpoint_desc(0x82, 0x03, 16, 1)
    )
);

auto conf_desc_250hz = configuration_desc(2, 1, 0, 0xc0, 0,
    // HID interface.
    interface_desc(0, 0, 1, 0x03, 0x00, 0x00, 0,
        hid_desc(0x111, 0, 1, 0x22, sizeof(report_desc)),
        endpoint_desc(0x81, 0x03, 16, 4)
    ),
    interface_desc(1, 0, 1, 0x03, 0x00, 0x00, 0,
        hid_desc(0x111, 0, 1, 0x22, sizeof(keyb_report_desc)),
        endpoint_desc(0x82, 0x03, 16, 4)
    )
);

desc_t dev_desc_p = {sizeof(dev_desc), (void*)&dev_desc};

desc_t conf_desc_p_1000hz = {sizeof(conf_desc_1000hz), (void*)&conf_desc_1000hz};
desc_t conf_desc_p_250hz = {sizeof(conf_desc_250hz), (void*)&conf_desc_250hz};

desc_t report_desc_p = {sizeof(report_desc), (void*)&report_desc};
desc_t keyb_report_desc_p =
    {sizeof(keyb_report_desc), (void*)&keyb_report_desc};

static Pin usb_dm = GPIOA[11];
static Pin usb_dp = GPIOA[12];
static Pin usb_pu = GPIOA[15];

static PinArray button_inputs = GPIOB.array(0, 10);
static PinArray button_leds = GPIOC.array(0, 6);
static Pin button8_led = GPIOC[7];
static Pin button9_led = GPIOC[8]; // do not use if ws2812b is active
static Pin start_led = GPIOC[9];
static Pin select_led = GPIOC[10];

static Pin qe1a = GPIOA[0];
static Pin qe1b = GPIOA[1];
static Pin qe2a = GPIOA[6];
static Pin qe2b = GPIOA[7];

static Pin led1 = GPIOA[8];
static Pin led2 = GPIOA[9];

USB_f1 usb_1000hz(USB, dev_desc_p, conf_desc_p_1000hz);
USB_f1 usb_250hz(USB, dev_desc_p, conf_desc_p_250hz);

bool global_led_enable = false;
bool global_tt_hid_enable = false;

RGBManager rgb_manager;

template <>
void interrupt<Interrupt::DMA1_Channel7>() {
    rgb_manager.irq();
}

uint32_t last_tt_led_time = 0;
timer hid_lights_expiry_timer;

class HID_arcin : public USB_HID {
    private:
        bool set_feature_bootloader(bootloader_report_t* report) {
            switch(report->func) {
                case 0:
                    return true;
                
                case 0x10: // Reset to bootloader
                    do_reset_bootloader = true;
                    return true;
                
                case 0x20: // Reset to runtime
                    do_reset = true;
                    return true;
                
                default:
                    return false;
            }
        }
        
        bool set_feature_config(config_report_t* report) {
            if(report->segment != 0) {
                return false;
            }
            
            configloader.write(report->size, report->data);
            
            return true;
        }
        
        bool get_feature_config() {
            config_report_t report = {0xc0, 0, sizeof(config)};
            
            memcpy(report.data, &config, sizeof(config));

            usb.write(0, (uint32_t*)&report, sizeof(report));
            
            return true;
        }
    
    public:
        HID_arcin(USB_generic& usbd, desc_t rdesc) : USB_HID(usbd, rdesc, 0, 1, 64) {}
    
    protected:
        virtual bool set_output_report(uint32_t* buf, uint32_t len) {
            if (len < sizeof(uint8_t)) {
                return false;
            }

            uint8_t report_id = *(uint8_t*)buf;
            if (report_id == 0x2 && len == sizeof(output_report_t)) {
                output_report_t* report = (output_report_t*)buf;
                set_hid_lights(report->leds);

            } else if (report_id == 0x3 &&
                       len == sizeof(output_report_rgb_t) &&
                       config.flags.Ws2812b) {

                output_report_rgb_t* report = (output_report_rgb_t*)buf;
                rgb_manager.update_from_hid(report->rgb);
            }

            return true;
        }
        
        virtual bool set_feature_report(uint32_t* buf, uint32_t len) {
            switch(*buf & 0xff) {
                case 0xb0:
                    if(len != sizeof(bootloader_report_t)) {
                        return false;
                    }
                    
                    return set_feature_bootloader((bootloader_report_t*)buf);
                
                case 0xc0:
                    if(len != sizeof(config_report_t)) {
                        return false;
                    }
                    
                    return set_feature_config((config_report_t*)buf);
                
                default:
                    return false;
            }
        }
        
        virtual bool get_feature_report(uint8_t report_id) {
            switch(report_id) {
                case 0xc0:
                    return get_feature_config();
                
                default:
                    return false;
            }
        }
};

class HID_keyb : public USB_HID {
    public:
        HID_keyb(USB_generic& usbd, desc_t rdesc) : USB_HID(usbd, rdesc, 1, 2, 64) {}

    protected:
        virtual bool set_output_report(uint32_t* buf, uint32_t len) {
            // ignore
            return true;
        }

        virtual bool set_feature_report(uint32_t* buf, uint32_t len) {
            // ignore
            return false;
        }
};

HID_arcin usb_hid_1000hz(usb_1000hz, report_desc_p);
HID_arcin usb_hid_250hz(usb_250hz, report_desc_p);

HID_keyb usb_hid_keyb_1000hz(usb_1000hz, keyb_report_desc_p);
HID_keyb usb_hid_keyb_250hz(usb_250hz, keyb_report_desc_p);

USB_strings usb_strings_1000hz(usb_1000hz, config.label);
USB_strings usb_strings_250hz(usb_250hz, config.label);

debounce_state debounce_state_raw;
debounce_state debounce_state_keys;
debounce_state debounce_state_effectors;
uint8_t debounce_window_effectors;

bool scheduled_leds = false;
uint32_t scheduled_led_time = 0;
uint16_t scheduled_leds_aside = 0;
uint16_t scheduled_leds_bside = 0;

void schedule_led(uint16_t time_from_now_ms, uint16_t leds_a, uint16_t leds_b) {
    scheduled_leds = true;
    scheduled_led_time = Time::time() + time_from_now_ms;
    scheduled_leds_aside = leds_a;
    scheduled_leds_bside = leds_b;
}

void set_button_lights(uint16_t leds) {
    button_leds.set(leds & ARCIN_PIN_BUTTON_ALL);
    button8_led.set((leds & ARCIN_PIN_BUTTON_8) != 0);
    if (!config.flags.Ws2812b) {
        button9_led.set((leds & ARCIN_PIN_BUTTON_9) != 0);
    }
    start_led.set((leds & ARCIN_PIN_BUTTON_START) != 0);
    select_led.set((leds & ARCIN_PIN_BUTTON_SELECT) != 0);
}

bool led1_last_state = false;
bool led2_last_state = false;
void set_tt_led(bool led1_on, bool led2_on) {
    if (!led1_last_state && led1_on) {
        led1.on();
    } else if (led1_last_state && !led1_on) {
        led1.off();
    }

    if (!led2_last_state && led2_on) {
        led2.on();
    } else if (led2_last_state && !led2_on) {
        led2.off();
    }

    led1_last_state = led1_on;
    led2_last_state = led2_on;
}

void set_hid_lights(uint16_t leds) {
    // if LED is globally disabled, ignore HID lights
    if (!global_led_enable) {
        return;
    }

    // if LED overrides are in effect, ignore HID lights
    if (scheduled_leds) {
        return;
    }

    // valid for 5 seconds
    hid_lights_expiry_timer.arm(5000);
    set_button_lights(leds);
    if (global_tt_hid_enable) {
        last_tt_led_time = last_led_time;
        set_tt_led((leds & 0x800) != 0, (leds & 0x1000) != 0);
    }
}

void check_for_outdated_tt_led_report(config_flags *runtime_flags) {
    if (runtime_flags->TtLedReactive) {
        return;
    }
    if (runtime_flags->TtLedHid &&
        last_tt_led_time != 0 &&
        (Time::time() - last_tt_led_time) < 5000) {
        return;
    }
    set_tt_led(global_led_enable, global_led_enable);
}

#if DEBUG_TIMING_GAMEPAD

#warning "----------- DEBUG OUTPUT ENABLED!!! -----------"
#warning "----------- DEBUG OUTPUT ENABLED!!! -----------"
#warning "----------- DEBUG OUTPUT ENABLED!!! -----------"
#warning "----------- DEBUG OUTPUT ENABLED!!! -----------"
#warning "----------- DEBUG OUTPUT ENABLED!!! -----------"

uint32_t previous_report_time = 0;
uint32_t debug_value;

#endif

int main() {
    rcc_init();
    
    // Initialize system timer.
    STK.LOAD = 72000000 / 8 / 1000; // 1000 Hz.
    STK.CTRL = 0x03;
    
    // Load config.
    configloader.read(sizeof(config), &config);

    config_flags runtime_flags = initialize_mode_switch(config.flags);

    RCC.enable(RCC.GPIOA);
    RCC.enable(RCC.GPIOB);
    RCC.enable(RCC.GPIOC);
    
    usb_dm.set_mode(Pin::AF);
    usb_dm.set_af(14);
    usb_dp.set_mode(Pin::AF);
    usb_dp.set_af(14);
    
    RCC.enable(RCC.USB);
    
    USB_f1* usb;
    if (runtime_flags.PollAt250Hz) {
        usb = &usb_250hz;
    } else {
        usb = &usb_1000hz;
    }

    usb->init();
    
    usb_pu.set_mode(Pin::Output);
    usb_pu.on();
    
    button_inputs.set_mode(Pin::Input);
    button_inputs.set_pull(Pin::PullUp);
    
    button_leds.set_mode(Pin::Output);
    button8_led.set_mode(Pin::Output);
    button9_led.set_mode(Pin::Output);
    start_led.set_mode(Pin::Output);
    select_led.set_mode(Pin::Output);
    
    led1.set_mode(Pin::Output);
    led2.set_mode(Pin::Output);
    set_tt_led(false, false);
    
    global_led_enable = !runtime_flags.LedOff;
    global_tt_hid_enable = runtime_flags.TtLedHid;
    
    RCC.enable(RCC.TIM2);
    RCC.enable(RCC.TIM3);
    
    if(!(runtime_flags.InvertQE1)) {
        TIM2.CCER = 1 << 1;
    }
    
    TIM2.CCMR1 = (1 << 8) | (1 << 0);
    TIM2.SMCR = 3;
    TIM2.CR1 = 1;
    
    if(config.qe1_sens < 0) {
        TIM2.ARR = 256 * -config.qe1_sens - 1;
    } else {
        TIM2.ARR = 256 - 1;
    }
    
    TIM3.CCMR1 = (1 << 8) | (1 << 0);
    TIM3.SMCR = 3;
    TIM3.CR1 = 1;
    
    if(config.qe2_sens < 0) {
        TIM3.ARR = 256 * -config.qe2_sens - 1;
    } else {
        TIM3.ARR = 256 - 1;
    }
    
    qe1a.set_af(1);
    qe1b.set_af(1);
    qe1a.set_mode(Pin::AF);
    qe1b.set_mode(Pin::AF);
    
    qe2a.set_af(2);
    qe2b.set_af(2);
    qe2a.set_mode(Pin::AF);
    qe2b.set_mode(Pin::AF);    

    analog_button tt1(4, 200, true);

    if (runtime_flags.DebounceEnable) {
        debounce_init(&debounce_state_keys, config.debounce_ticks);
    }

    // effectors always have a little bit of debouncing enabled
    debounce_window_effectors = 4;

    // Take the higher value if user has debouncing enabled
    if (runtime_flags.DebounceEnable) {
        debounce_window_effectors =
            max(debounce_window_effectors, config.debounce_ticks);
    }

    debounce_init(&debounce_state_effectors, debounce_window_effectors);

    // debounce for raw input
    debounce_init(&debounce_state_raw, 4);

    // Init done, flash some lights for 1 second
    schedule_led(1000, ARCIN_PIN_BUTTON_WHITE, ARCIN_PIN_BUTTON_WHITE);

    if (config.flags.Ws2812b) {
        // turn on the power before initializing
        button9_led.on();
        // must be called last
        rgb_manager.init(&config.rgb);
    }

    while(1) {
        usb->process();

        uint32_t now = Time::time();
        uint16_t buttons = button_inputs.get() ^ 0x7ff;
        if (config.flags.Ws2812b) {
            buttons &= (~ARCIN_PIN_BUTTON_9);
        }
        
        if(do_reset_bootloader) {
            Time::sleep(10);
            reset_bootloader();
        }
        
        if(do_reset) {
            Time::sleep(10);
            reset();
        }
        
        // Non-HID controlled handling of button LEDs
        if (scheduled_leds) {
            int32_t diff = scheduled_led_time - now;
            if (diff > 0) {
                if ((diff / 200) % 2 == 0) {
                    set_button_lights(scheduled_leds_aside);
                } else {
                    set_button_lights(scheduled_leds_bside);
                }
            } else {
                // we went past it; no more scheduled lights
                scheduled_leds = false;
            }

        } else if (
            !hid_lights_expiry_timer.is_armed() ||
            hid_lights_expiry_timer.check_if_expired_reset()) {

            // If it's been a while since the last HID lights, use the raw
            // button input for lights
            if (global_led_enable) {
                set_button_lights(buttons);
            } else {
                set_button_lights(0);
            }
        }

        // Non-HID controlled handling of LED1 / LED2
        check_for_outdated_tt_led_report(&runtime_flags);

        // [READ QE1]
        uint32_t qe1_count = TIM2.CNT;

        // [MODE] Apply debounce to raw input & process runtime mode switching
        if (runtime_flags.ModeSwitchEnable) {
            uint16_t raw_debounced = buttons;
            uint16_t debounce_mask =
                (INFINITAS_BUTTON_ALL | INFINITAS_EFFECTORS_ALL);
            raw_debounced =
                (buttons & ~debounce_mask) |
                (debounce(&debounce_state_raw, buttons & debounce_mask));

            runtime_flags = process_mode_switch(raw_debounced);

            // Update LED options state.
            global_led_enable = !runtime_flags.LedOff;
        }

        // [REMAP]
        uint16_t remapped = remap_buttons(config, buttons);

        // [DEBOUNCE] Apply debounce to remapped keys
        if (runtime_flags.DebounceEnable) {
            uint16_t debounce_mask = INFINITAS_BUTTON_ALL;
            remapped =
                (remapped & ~debounce_mask) |
                (debounce(&debounce_state_keys, remapped & debounce_mask));
        }

        // [DEBOUNCE] Apply debounce to remapped effectors
        {
            uint16_t debounce_mask = INFINITAS_EFFECTORS_ALL;
            remapped =
                (remapped & ~debounce_mask) |
                (debounce(&debounce_state_effectors, remapped & debounce_mask));
        }

        // [DIGITAL QE1]
        int8_t tt1_report = 0;
        tt1_report = tt1.poll(qe1_count);

        // [DIGITAL QE1 POST-PROCESSING]
        if (runtime_flags.TtLedReactive) {
            if (global_led_enable) {
                switch (tt1_report) {
                    case -1:
                    case 1:
                        set_tt_led(true, true);
                        break;
                    default:
                        set_tt_led(false, false);
                        break;
                }
            } else {
                set_tt_led(false, false);    
            }
        }

        if (config.flags.Ws2812b) {
            rgb_manager.update_colors(-tt1_report);
        }

        // [E2 MULTI-TAP]
        // Multi-tap processing of E2. Must be done after debounce.
        if (runtime_flags.SelectMultiFunction) {
            // Always clear E2 since it should not be asserted directly
            bool is_e2_pressed = (remapped & INFINITAS_BUTTON_E2) != 0;
            remapped &= ~(INFINITAS_BUTTON_E2);
            remapped |= get_multi_function_keys(is_e2_pressed);
        }

        // [GAMEPAD]]
        if (usb->ep_ready(1)) {
            input_report_t report;
            report.report_id = 1;

            // [Joy Buttons report]
            if (runtime_flags.JoyInputForceDisable) {
                report.buttons = 0;
            } else {
                // [DIGITAL TT -> BUTTONS]
                if (runtime_flags.DigitalTTEnable) {
                    switch (tt1_report) {
                    case -1:
                        remapped |= JOY_BUTTON_13;
                        break;
                    case 1:
                        remapped |= JOY_BUTTON_14;
                        break;
                    default:
                        break;
                    }
                }

                report.buttons = remapped;
            }

            // [X-axis report]
            if (runtime_flags.JoyInputForceDisable ||
                (runtime_flags.DigitalTTEnable && !runtime_flags.AnalogTTForceEnable)) {
                report.axis_x = uint8_t(127);
            } else {
                // [ANALOG TT -> SENSITIVITY]
                // Adjust turntable sensitivity. Must be done AFTER digital TT
                // processing.
                if (config.qe1_sens < 0) {
                    qe1_count /= -config.qe1_sens;
                } else if (config.qe1_sens > 0) {
                    qe1_count *= config.qe1_sens;
                }

                if (analog_tt_reverse_direction) {
                    report.axis_x = uint8_t(255 - qe1_count);
                } else {
                    report.axis_x = uint8_t(qe1_count);
                }
            }

            report.axis_y = 127;

#if DEBUG_TIMING_GAMEPAD

            uint32_t nownow = Time::time();
            uint32_t delta = nownow - previous_report_time;
             report.buttons = (debug_value >> 16);
             report.axis_x = (debug_value >> 8);
             report.axis_y = debug_value;
            // report.buttons = (delta >> 16);
            // report.axis_x = (delta >> 8);
            // report.axis_y = delta;
            previous_report_time = nownow;

#endif

            usb->write(1, (uint32_t*)&report, sizeof(report));
        }
        
        // [KEYBOARD]]
        if (usb->ep_ready(2)) {
            unsigned char scancodes[13] = { 0 };

            static_assert(
                ARRAY_SIZE(infinitas_keys) + 1 <=
                ARRAY_SIZE(scancodes),
                "keycode array too small");

            uint8_t nextscan = 0;

            if (runtime_flags.KeyboardEnable) {
                for (uint8_t i = 0; i < ARRAY_SIZE(infinitas_keys); i++) {
                    if (remapped & infinitas_keys[i]) {
                        scancodes[nextscan++] = config.keycodes[i];
                    }
                }

                switch (tt1_report) {
                case -1:
                    scancodes[nextscan++] = config.keycodes[11];
                    break;
                case 1:
                    scancodes[nextscan++] = config.keycodes[12];
                    break;
                default:
                    break;
                }
            }

            usb->write(2, (uint32_t*)scancodes, sizeof(scancodes));
        }
    }
}
