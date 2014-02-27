#include <rcc/rcc.h>
#include <rcc/flash.h>
#include <gpio/gpio.h>
#include <interrupt/interrupt.h>
#include <timer/timer.h>
#include <os/time.h>
#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/hid.h>

#include <string.h>

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

class Configloader {
	private:
		enum {
			MAGIC = 0xc0ff600d,
		};
		
		struct header_t {
			uint32_t magic;
			uint32_t size;
		};
		
		uint32_t flash_addr;
		
	public:
		Configloader(uint32_t addr) : flash_addr(addr) {}
		
		bool read(uint32_t size, void* data) {
			header_t* header = (header_t*)flash_addr;
			
			if(header->magic != MAGIC) {
				return false;
			}
			
			if(header->size < size) {
				size = header->size;
			}
			
			memcpy(data, (void*)(flash_addr + sizeof(header_t)), size);
			
			return true;
		}
		
		bool write(uint32_t size, void* data) {
			header_t header = {MAGIC, size};
			
			// Unlock flash.
			FLASH.KEYR = 0x45670123;
			FLASH.KEYR = 0xCDEF89AB;
			
			// Erase page.
			FLASH.CR = 1 << 1; // PER
			FLASH.AR = flash_addr;
			FLASH.CR = (1 << 6) | (1 << 1); // STRT, PER
			
			while(FLASH.SR & (1 << 0)); // BSY
			
			FLASH.SR &= ~(1 << 5); // EOP
			FLASH.CR = 0;
			
			// Write header.
			uint16_t* src = (uint16_t*)&header;
			uint16_t* dest = (uint16_t*)flash_addr;
			
			for(uint32_t n = 0; n < sizeof(header); n += 2) {
				FLASH.CR = 1 << 0; // PG
				
				*dest++ = *src++;
				
				while(FLASH.SR & (1 << 0)); // BSY
			}
			
			// Write data.
			src = (uint16_t*)data;
			
			for(uint32_t n = 0; n < size; n += 2) {
				FLASH.CR = 1 << 0; // PG
				
				*dest++ = *src++;
				
				while(FLASH.SR & (1 << 0)); // BSY
			}
			
			// Lock flash.
			FLASH.CR = 1 << 7; // LOCK
			
			return true;
		}
};

Configloader configloader(0x801f800);

struct config_t {
	uint8_t label[12];
	uint32_t flags;
	int8_t qe1_sens;
	int8_t qe2_sens;
	uint8_t ps2_mode;
	uint8_t ws2812b_mode;
};

config_t config;

auto report_desc = gamepad(
	// Inputs.
	report_id(1),
	
	buttons(11),
	padding_in(5),
	
	usage_page(UsagePage::Desktop),
	usage(DesktopUsage::X),
	logical_minimum(0),
	logical_maximum(255),
	report_count(1),
	report_size(8),
	input(0x02),

	usage_page(UsagePage::Desktop),
	usage(DesktopUsage::Y),
	logical_minimum(0),
	logical_maximum(255),
	report_count(1),
	report_size(8),
	input(0x02),
	
	// Outputs.
	report_id(2),
	
	usage_page(UsagePage::Ordinal),
	usage(1),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(2),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(3),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(4),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(5),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(6),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(7),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(8),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(9),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(10),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	usage_page(UsagePage::Ordinal),
	usage(11),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	padding_out(5),
	
	// Bootloader
	report_id(0xb0),
	
	usage_page(0xff55),
	usage(0xb007),
	logical_minimum(0),
	logical_maximum(255),
	report_size(8),
	report_count(1),
	
	feature(0x02), // HID bootloader function
	
	// Configuration
	report_id(0xc0),
	
	usage(0xc000),
	feature(0x02), // Config segment
	
	usage(0xc001),
	feature(0x02), // Config segment size
	
	feature(0x01), // Padding
	
	usage(0xc0ff),
	report_count(60),
	feature(0x02) // Config data
);

struct input_report_t {
	uint8_t report_id;
	uint16_t buttons;
	uint8_t axis_x;
	uint8_t axis_y;
} __attribute__((packed));

struct output_report_t {
	uint8_t report_id;
	uint16_t leds;
} __attribute__((packed));

struct bootloader_report_t {
	uint8_t report_id;
	uint8_t func;
} __attribute__((packed));

struct config_report_t {
	uint8_t report_id;
	uint8_t segment;
	uint8_t size;
	uint8_t pad;
	uint8_t data[60];
} __attribute__((packed));

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

uint32_t serial_num() {
	uint32_t* uid = (uint32_t*)0x1ffff7ac;
	
	return uid[0] * uid[1] * uid[2];
}

class USB_strings : public USB_class_driver {
	private:
		USB_generic& usb;
		
	public:
		USB_strings(USB_generic& usbd) : usb(usbd) {
			usb.register_driver(this);
		}
	
	protected:
		virtual SetupStatus handle_setup(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			// Get string descriptor.
			if(bmRequestType == 0x80 && bRequest == 0x06 && (wValue & 0xff00) == 0x0300) {
				const void* desc = nullptr;
				uint16_t buf[64] = {0x300};
				uint32_t i = 1;
				
				switch(wValue & 0xff) {
					case 0:
						desc = u"\u0304\u0409";
						break;
					
					case 1:
						desc = u"\u0308zyp";
						break;
					
					case 2:
						for(const char* p = "arcin"; *p; p++) {
							buf[i++] = *p;
						}
						
						if(config.label[0]) {
							buf[i++] = ' ';
							buf[i++] = '(';
							
							for(uint8_t* p = config.label; *p; p++) {
								buf[i++] = *p;
							}
							
							buf[i++] = ')';
						}
						
						buf[0] |= i * 2;
						
						desc = buf;
						break;
					
					case 3:
						{
							buf[0] = 0x0312;
							uint32_t id = serial_num();
							for(int i = 8; i > 0; i--) {
								buf[i] = (id & 0xf) > 9 ? 'A' + (id & 0xf) - 0xa : '0' + (id & 0xf);
								id >>= 4;
							}
							desc = buf;
						}
						break;
				}
				
				if(!desc) {
					return SetupStatus::Unhandled;
				}
				
				uint8_t len = *(uint8_t*)desc;
				
				if(len > wLength) {
					len = wLength;
				}
				
				usb.write(0, (uint32_t*)desc, len);
				
				return SetupStatus::Ok;
			}
			
			return SetupStatus::Unhandled;
		}
};

USB_strings usb_strings(usb);

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
	
	TIM2.CCMR1 = (1 << 8) | (1 << 0);
	TIM2.CCER = 1 << 1;
	TIM2.SMCR = 3;
	TIM2.CR1 = 1;
	
	if(config.qe1_sens < 0) {
		TIM2.ARR = 256 * -config.qe1_sens - 1;
	} else {
		TIM2.ARR = 256 - 1;
	}
	
	TIM3.CCMR1 = (1 << 8) | (1 << 0);
	TIM3.CCER = 1 << 1;
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
