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

auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1d50, 0x6080, 0x110, 1, 2, 3, 1);
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
	
	if(!(config.flags & (1 << 1))) {
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
	
	if(!(config.flags & (1 << 2))) {
		TIM3.CCER = 1 << 1;
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
	
	uint8_t last_x = 0;
	uint8_t last_y = 0;
	
	int8_t state_x = 0;
	int8_t state_y = 0;
	
	while(1) {
		usb.process();
		
		uint16_t buttons = button_inputs.get() ^ 0x7ff;
		
		if(do_reset_bootloader) {
			Time::sleep(10);
			reset_bootloader();
		}
		
		if(do_reset) {
			Time::sleep(10);
			reset();
		}
		
		if(Time::time() - last_led_time > 1000) {
			button_leds.set(buttons);
		}
		
		if(usb.ep_ready(1)) {
			uint32_t qe1_count = TIM2.CNT;
			uint32_t qe2_count = TIM3.CNT;
			
			int8_t rx = qe1_count - last_x;
			int8_t ry = qe2_count - last_y;
			
			if(rx > 1) {
				state_x = 100;
				last_x = qe1_count;
			} else if(rx < -1) {
				state_x = -100;
				last_x = qe1_count;
			} else if(state_x > 0) {
				state_x--;
				last_x = qe1_count;
			} else if(state_x < 0) {
				state_x++;
				last_x = qe1_count;
			}
			
			if(ry > 1) {
				state_y = 100;
				last_y = qe2_count;
			} else if(ry < -1) {
				state_y = -100;
				last_y = qe2_count;
			} else if(state_y > 0) {
				state_y--;
				last_y = qe2_count;
			} else if(state_y < 0) {
				state_y++;
				last_y = qe2_count;
			}
			
			if(state_x > 0) {
				buttons |= 1 << 11;
			} else if(state_x < 0) {
				buttons |= 1 << 12;
			}
			
			if(state_y > 0) {
				buttons |= 1 << 13;
			} else if(state_y < 0) {
				buttons |= 1 << 14;
			}
			
			if(config.qe1_sens < 0) {
				qe1_count /= -config.qe1_sens;
			} else if(config.qe1_sens > 0) {
				qe1_count *= config.qe1_sens;
			}
			
			if(config.qe2_sens < 0) {
				qe2_count /= -config.qe2_sens;
			} else if(config.qe2_sens > 0) {
				qe2_count *= config.qe2_sens;
			}
			
			input_report_t report = {1, buttons, uint8_t(qe1_count), uint8_t(qe2_count)};
			
			usb.write(1, (uint32_t*)&report, sizeof(report));
		}
	}
}
