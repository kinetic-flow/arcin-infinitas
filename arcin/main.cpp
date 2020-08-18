#include <rcc/rcc.h>
#include <gpio/gpio.h>
#include <interrupt/interrupt.h>
#include <timer/timer.h>
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
auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1CCF, 0x8048, 0x110, 1, 2, 3, 1);

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
static PinArray button_leds = GPIOC.array(0, 10);

static Pin qe1a = GPIOA[0];
static Pin qe1b = GPIOA[1];
static Pin qe2a = GPIOA[6];
static Pin qe2b = GPIOA[7];

static Pin led1 = GPIOA[8];
static Pin led2 = GPIOA[9];

USB_f1 usb_1000hz(USB, dev_desc_p, conf_desc_p_1000hz);
USB_f1 usb_250hz(USB, dev_desc_p, conf_desc_p_250hz);

uint32_t last_led_time;

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
            if(len != sizeof(output_report_t)) {
                return false;
            }
            
            output_report_t* report = (output_report_t*)buf;
            
            set_hid_lights(report->leds);
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

class analog_button {
public:
    // config

    // Number of ticks we need to advance before recognizing an input
    uint32_t deadzone;
    // How long to sustain the input before clearing it (if opposite direction is input, we'll release immediately)
    uint32_t sustain_ms;
    // Always provide a zero-input for one poll before reversing?
    bool clear;

    // State: Center of deadzone
    uint32_t center;
    bool center_valid;
    // times to: reset to zero, reset center to counter
    uint32_t t_timeout;

    int8_t state; // -1, 0, 1
    int8_t last_delta;
public:
    analog_button(uint32_t deadzone, uint32_t sustain_ms, bool clear)
        : deadzone(deadzone), sustain_ms(sustain_ms), clear(clear)
    {
        center = 0;
        center_valid = false;
        t_timeout = 0;
        state = 0;
    }

    int8_t poll(uint32_t current_value) {
        if (!center_valid) {
            center_valid = true;
            center = current_value;
        }

        uint8_t observed = current_value;
        int8_t delta = observed - center;
        last_delta = delta;

        uint8_t direction = 0;
        if (delta >= (int32_t)deadzone) {
            direction = 1;
        } else if (delta <= -(int32_t)deadzone) {
            direction = -1;
        }

        if (direction != 0) {
            center = observed;
            t_timeout = Time::time() + sustain_ms;
        } else if (t_timeout != 0 && Time::time() >= t_timeout) {
            state = 0;
            center = observed;
            t_timeout = 0;
        }

        if (direction == -state && clear) {
            state = direction;
            return 0;
        } else if (direction != 0) {
            state = direction;
        }

        return state;
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

uint32_t scheduled_led_time = 0;
uint16_t scheduled_leds_aside = 0;
uint16_t scheduled_leds_bside = 0;
void schedule_led(uint32_t end_time, uint16_t leds_a, uint16_t leds_b) {
    // If scheduling in the past, nothing to do
    // This also guards against wallclock rollover
    if (end_time < Time::time()) {
        return;
    }

    scheduled_led_time = end_time;
    scheduled_leds_aside = leds_a;
    scheduled_leds_bside = leds_b;
}

void set_hid_lights(uint16_t leds) {
    // if LED overrides are in effect, ignore HID lights
    if (Time::time() < scheduled_led_time) {
        return;
    }

    last_led_time = Time::time();
    button_leds.set(leds);
}

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
    
    led1.set_mode(Pin::Output);
    led1.on();
    
    led2.set_mode(Pin::Output);
    led2.on();
    
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

    // Init done, flash some lights for 2 seconds
    schedule_led(
        Time::time() + 1000, ARCIN_PIN_BUTTON_WHITE, ARCIN_PIN_BUTTON_WHITE);

    while(1) {
        usb->process();

        uint32_t now = Time::time();
        uint16_t buttons = button_inputs.get() ^ 0x7ff;
        
        if(do_reset_bootloader) {
            Time::sleep(10);
            reset_bootloader();
        }
        
        if(do_reset) {
            Time::sleep(10);
            reset();
        }
        
        if (now < scheduled_led_time) {
            uint16_t diff = now - scheduled_led_time;
            if ((diff / 200) % 2 == 0) {
                button_leds.set(scheduled_leds_aside);
            } else {
                button_leds.set(scheduled_leds_bside);
            }
        } else if (now - last_led_time > 1000) {
            // If it's been a while since the last HID lights, use the raw
            // button input for lights
            button_leds.set(buttons);
        }

        // [READ QE1]
        uint32_t qe1_count = TIM2.CNT;

        // [MODE] Apply debounce to raw input & process runtime mode switching
        uint16_t raw_debounced = buttons;
        {
            uint16_t debounce_mask =
                (INFINITAS_BUTTON_ALL | INFINITAS_EFFECTORS_ALL);
            raw_debounced =
                (buttons & ~debounce_mask) |
                (debounce(&debounce_state_raw, buttons & debounce_mask));

            runtime_flags = process_mode_switch(raw_debounced);
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
        if (runtime_flags.DigitalTTEnable || runtime_flags.KeyboardEnable) {
            tt1_report = tt1.poll(qe1_count);
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
                report.axis_x = uint8_t(qe1_count);
            }

            report.axis_y = 127;
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
