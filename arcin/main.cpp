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

//
// Pin out on arcin board
//

#define ARCIN_BUTTON_KEY_1 	      ((uint16_t)(1 << 0))
#define ARCIN_BUTTON_KEY_2        ((uint16_t)(1 << 1))
#define ARCIN_BUTTON_KEY_3        ((uint16_t)(1 << 2))
#define ARCIN_BUTTON_KEY_4        ((uint16_t)(1 << 3))
#define ARCIN_BUTTON_KEY_5        ((uint16_t)(1 << 4))
#define ARCIN_BUTTON_KEY_6        ((uint16_t)(1 << 5))
#define ARCIN_BUTTON_KEY_7        ((uint16_t)(1 << 6))

#define ARCIN_BUTTON_KEY_ALL_MAIN ((uint16_t)(0x7F))

#define ARCIN_BUTTON_EXTRA_8      ((uint16_t)(1 << 7))
#define ARCIN_BUTTON_EXTRA_9      ((uint16_t)(1 << 8))

#define ARCIN_BUTTON_START        ((uint16_t)(1 << 9))  // button 10
#define ARCIN_BUTTON_SEL   	      ((uint16_t)(1 << 10)) // button 11

//
// Remapped values for Windows
//

#define INFINITAS_DIGITAL_TT_CW   ((uint16_t)(1 << 12))  // button 13
#define INFINITAS_DIGITAL_TT_CCW  ((uint16_t)(1 << 13))  // button 14

#define INFINITAS_BUTTON_E1       ((uint16_t)(1 << 8))  // E1
#define INFINITAS_BUTTON_E2       ((uint16_t)(1 << 9))  // E2
#define INFINITAS_BUTTON_E3       ((uint16_t)(1 << 10)) // E3
#define INFINITAS_BUTTON_E4       ((uint16_t)(1 << 11)) // E4

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

auto conf_desc = configuration_desc(1, 1, 0, 0xc0, 0,
	// HID interface.
	interface_desc(0, 0, 1, 0x03, 0x00, 0x00, 0,
		hid_desc(0x111, 0, 1, 0x22, sizeof(report_desc)),
		endpoint_desc(0x81, 0x03, 16, 1)
	)
);

desc_t dev_desc_p = {sizeof(dev_desc), (void*)&dev_desc};
desc_t conf_desc_p = {sizeof(conf_desc), (void*)&conf_desc};
desc_t report_desc_p = {sizeof(report_desc), (void*)&report_desc};

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

USB_f1 usb(USB, dev_desc_p, conf_desc_p);

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
			
			last_led_time = Time::time();
			button_leds.set(report->leds);
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

uint16_t ARCIN_DEBUG;

uint32_t first_e2_falling_edge_time;

#define LAST_E2_STATUS_SIZE 4

bool last_e2_status[LAST_E2_STATUS_SIZE]; // [0] is most recent, [1] is one before that, and so on
uint32_t e2_falling_edge_count;

// Window that begins on the first falling edge of E2. At the end of the window
// the number of taps is calculated and the resulting button combination will
// begin to assert.
// i.e., any multi-taps must be done within this window in order to count
#define MULTITAP_DETECTION_WINDOW_MS 		  400
#define MULTITAP_RESULT_HOLD_MS   100
#define MULTITAP_IGNORE_WINDOW_MS 300

uint16_t multitap_active_frames;
uint16_t multitap_buttons_to_assert;

// For LR2 mode digital turntable
#define DIGITAL_TT_HOLD_DURATION_MS 100

uint8_t last_x = 0;
int16_t state_x = 0;

void e2_update(bool pressed) {
	// falling edge (detect on-on-off-off-off sequence)
	if (!pressed &&
		!last_e2_status[0] &&
		!last_e2_status[1] &&
		last_e2_status[2] &&
		last_e2_status[3]) {

		// start counting on the first falling edge
		if (first_e2_falling_edge_time == 0) {
			first_e2_falling_edge_time = Time::time();
		}

		e2_falling_edge_count += 1;
	}

	for (int i = (LAST_E2_STATUS_SIZE - 1); i >= 1; i -= 1) {
		last_e2_status[i] = last_e2_status[i-1];
	}

	last_e2_status[0] = pressed;
}

uint16_t get_multitap_output(uint32_t tap_count) {
	uint16_t button;
	switch (tap_count) {
	case 1:
		button = INFINITAS_BUTTON_E2;
		break;

	case 2:
		button = INFINITAS_BUTTON_E3;
		break;

	case 3:
		button = (INFINITAS_BUTTON_E2 | INFINITAS_BUTTON_E3);
		break;

	case 0:
	default:
		button = 0;
		break;
	}

	return button;
}

bool is_multitap_window_closed() {
	uint32_t diff;

	if (first_e2_falling_edge_time == 0) {
		return false;
	}

	diff = Time::time() - first_e2_falling_edge_time;
	return diff > MULTITAP_DETECTION_WINDOW_MS;
}

uint16_t remap_buttons(uint16_t buttons) {
	uint16_t remapped;

	// Grab the first 7 buttons (keys)
	remapped = buttons & ARCIN_BUTTON_KEY_ALL_MAIN;

	// Remap start button
	if (buttons & ARCIN_BUTTON_START) {
		switch(config.effector_mode) {
		case START_E1_SEL_E2:
		default:
			remapped |= INFINITAS_BUTTON_E1;
			break;

		case START_E2_SEL_E1:
			remapped |= INFINITAS_BUTTON_E2;
			break;

		case START_E3_SEL_E4:
			remapped |= INFINITAS_BUTTON_E3;
			break;

		case START_E4_SEL_E3:
			remapped |= INFINITAS_BUTTON_E4;
			break;
		}
	}

	// Remap select button
	if (buttons & ARCIN_BUTTON_SEL) {
		switch(config.effector_mode) {
		case START_E1_SEL_E2:
		default:
			remapped |= INFINITAS_BUTTON_E2;
			break;

		case START_E2_SEL_E1:
			remapped |= INFINITAS_BUTTON_E1;
			break;

		case START_E3_SEL_E4:
			remapped |= INFINITAS_BUTTON_E4;
			break;

		case START_E4_SEL_E3:
			remapped |= INFINITAS_BUTTON_E3;
			break;
		}
	}

	// Button 8 is normally E3, unless flipped
	if (buttons & ARCIN_BUTTON_EXTRA_8) {
		if (config.flags & ARCIN_CONFIG_FLAG_SWAP_8_9) {
			remapped |= INFINITAS_BUTTON_E4;
		} else {
			remapped |= INFINITAS_BUTTON_E3;
		}
	}

	// Button 9 is normally E4, unless flipped
	if (buttons & ARCIN_BUTTON_EXTRA_9) {
		if (config.flags & ARCIN_CONFIG_FLAG_SWAP_8_9) {
			remapped |= INFINITAS_BUTTON_E3;
		} else {
			remapped |= INFINITAS_BUTTON_E4;
		}
	}

	return remapped;
}

HID_arcin usb_hid(usb, report_desc_p);

USB_strings usb_strings(usb, config.label);

int main() {
	rcc_init();
	
	// Initialize system timer.
	STK.LOAD = 72000000 / 8 / 1000; // 1000 Hz.
	STK.CTRL = 0x03;
	
	// Load config.
	configloader.read(sizeof(config), &config);

	RCC.enable(RCC.GPIOA);
	RCC.enable(RCC.GPIOB);
	RCC.enable(RCC.GPIOC);
	
	usb_dm.set_mode(Pin::AF);
	usb_dm.set_af(14);
	usb_dp.set_mode(Pin::AF);
	usb_dp.set_af(14);
	
	RCC.enable(RCC.USB);
	
	usb.init();
	
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
	
	if(!(config.flags & ARCIN_CONFIG_FLAG_INVERT_QE1)) {
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

	while(1) {
		usb.process();

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
		
		if(now - last_led_time > 1000) {
			button_leds.set(buttons);
		}

		if (ARCIN_DEBUG > 0) {
			button_leds.set(ARCIN_DEBUG);
		}
		
		if(usb.ep_ready(1)) {
			uint32_t qe1_count = TIM2.CNT;
			uint16_t remapped = remap_buttons(buttons);

			if (config.flags & ARCIN_CONFIG_FLAG_SEL_MULTI_TAP) {
				if (multitap_active_frames == 0) {
					// Make a note of its current state
					e2_update((remapped & INFINITAS_BUTTON_E2) != 0);
				}

				// Always clear E2 since it should not be asserted directly
				remapped &= ~(INFINITAS_BUTTON_E2);

				if (multitap_active_frames > 0) {
					
					// First, assert the resulting button combination
					// Then, enter the "ignore" window where E2 taps are ignored
					// for a bit.
					if (multitap_active_frames > MULTITAP_IGNORE_WINDOW_MS) {
						remapped |= multitap_buttons_to_assert;
					}

					multitap_active_frames--;

				} else if (is_multitap_window_closed()) {
					multitap_active_frames =
						(MULTITAP_RESULT_HOLD_MS + MULTITAP_IGNORE_WINDOW_MS);

					first_e2_falling_edge_time = 0;
					multitap_buttons_to_assert =
						get_multitap_output(e2_falling_edge_count);

					e2_falling_edge_count = 0;
				}
			}

			// digital turntable
			if (config.flags & ARCIN_CONFIG_FLAG_DIGITAL_TT_ENABLE) {
				int8_t rx = qe1_count - last_x;
				if(rx > 1) {
					state_x = DIGITAL_TT_HOLD_DURATION_MS;
					last_x = qe1_count;
				} else if(rx < -1) {
					state_x = -DIGITAL_TT_HOLD_DURATION_MS;
					last_x = qe1_count;
				} else if(state_x > 0) {
					state_x--;
					last_x = qe1_count;
				} else if(state_x < 0) {
					state_x++;
					last_x = qe1_count;
				}

				if(state_x > 0) {
					remapped |= INFINITAS_DIGITAL_TT_CCW;
				} else if(state_x < 0) {
					remapped |= INFINITAS_DIGITAL_TT_CW;
				}
			}

			if(config.qe1_sens < 0) {
				qe1_count /= -config.qe1_sens;
			} else if(config.qe1_sens > 0) {
				qe1_count *= config.qe1_sens;
			}

			input_report_t report;
			report.report_id = 1;
			report.buttons = remapped;
			if (config.flags & ARCIN_CONFIG_FLAG_DIGITAL_TT_ENABLE) {
				report.axis_x = uint8_t(127);
			} else {
				report.axis_x = uint8_t(qe1_count);
			}
			report.axis_y = 127;
			usb.write(1, (uint32_t*)&report, sizeof(report));
		}
	}
}
