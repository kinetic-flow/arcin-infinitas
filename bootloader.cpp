#include <rcc/rcc.h>
#include <rcc/flash.h>
#include <gpio/gpio.h>
#include <interrupt/interrupt.h>
#include <os/time.h>
#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/hid.h>

static uint32_t& reset_reason = *(uint32_t*)0x10000000;
static const uint32_t* firmware_vtors = (uint32_t*)0x8002000;

static bool do_reset;

void reset() {
	SCB.AIRCR = (0x5fa << 16) | (1 << 2); // SYSRESETREQ
}

void chainload(uint32_t offset) {
	SCB.VTOR = offset;
	
	asm volatile("ldr sp, [%0]; ldr %0, [%0, #4]; bx %0" :: "r" (offset));
}

auto report_desc = pack(
		usage_page(0xff55),
		usage(0xb007),
		collection(Collection::Application,
			logical_minimum(0),
			logical_maximum(255),
			report_size(8),
			report_count(1),
			
			usage(0xb007),
			input(0x02), // Status
			
			usage(0xb007),
			feature(0x02), // Function
			
			usage(0xb007),
			report_count(64),
			output(0x02) // Data
		)
);

auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1d50, 0x6084, 0x110, 1, 2, 3, 1);
auto conf_desc = configuration_desc(1, 1, 0, 0xc0, 0,
	// HID interface.
	interface_desc(0, 0, 1, 0x03, 0x00, 0x00, 0,
		hid_desc(0x111, 0, 1, 0x22, sizeof(report_desc)),
		endpoint_desc(0x81, 0x03, 64, 1)
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

static Pin led1 = GPIOA[8];
static Pin led2 = GPIOA[9];

USB_f1 usb(USB, dev_desc_p, conf_desc_p);

class Flashloader {
	private:
		bool state;
		uint32_t addr;
	
	public:
		Flashloader() : state(false) {}
		
		bool prepare() {
			addr = 0x8002000;
			state = true;
			
			// Unlock flash.
			FLASH.KEYR = 0x45670123;
			FLASH.KEYR = 0xCDEF89AB;
			
			return true;
		}
		
		bool write_block(uint32_t size, void* data) {
			if(!state) {
				return false;
			}
			
			if(size & 1) {
				return false;
			}
			
			if(addr + size > 0x8020000) {
				return false;
			}
			
			if(!(addr & (2048 - 1))) {
				// Erase page.
				
				FLASH.CR = 1 << 1; // PER
				FLASH.AR = addr;
				FLASH.CR = (1 << 6) | (1 << 1); // STRT, PER
				
				while(FLASH.SR & (1 << 0)); // BSY
				
				FLASH.SR &= ~(1 << 5); // EOP
				FLASH.CR = 0;
			}
			
			uint16_t* src = (uint16_t*)data;
			uint16_t* dest = (uint16_t*)addr;
			
			for(uint32_t n = 0; n < size; n += 2) {
				FLASH.CR = 1 << 0; // PG
				
				*dest++ = *src++;
				
				while(FLASH.SR & (1 << 0)); // BSY
			}
			
			
			addr += size;
			return true;
		}
		
		bool finish() {
			state = false;
			
			FLASH.CR = 1 << 7; // LOCK
			
			return true;
		}
};

Flashloader flashloader;

class HID_bootloader : public USB_HID {
	public:
		HID_bootloader(USB_generic& usbd, desc_t rdesc) : USB_HID(usbd, rdesc, 0, 1, 64) {}
	
	protected:
		virtual bool set_output_report(uint32_t* buf, uint32_t len) {
			if(len != 64) {
				return false;
			}
			
			return flashloader.write_block(len, buf);
		}
		
		virtual bool set_feature_report(uint32_t* buf, uint32_t len) {
			if(len != 1) {
				return false;
			}
			
			switch(*buf & 0xff) {
				case 0:
					return true;
				
				case 0x10: // Reset to bootloader
					return false; // Not available in bootloader mode
				
				case 0x11: // Reset to runtime
					do_reset = true;
					return true;
				
				case 0x20: // Flash prepare
					return flashloader.prepare();
				
				case 0x21: // Flash finish
					return flashloader.finish();
				
				default:
					return false;
			}
		}
};

HID_bootloader usb_hid(usb, report_desc_p);

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
				uint16_t buf[9];
				
				switch(wValue & 0xff) {
					case 0:
						desc = u"\u0304\u0409";
						break;
					
					case 1:
						desc = u"\u0308zyp";
						break;
					
					case 2:
						desc = u"\u0322arcin bootloader";
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

bool normal_boot() {
	// Check if this was a reset-to-bootloader.
	if(reset_reason == 0xb007) {
		reset_reason = 0;
		return false;
	}
	
	// Check buttons.
	if((button_inputs.get() ^ 0x7ff) == ((1 << 1) | (1 << 0))) {
		return false;
	}
	
	// Check that reset vector is a valid flash address.
	uint32_t reset_vector = firmware_vtors[1];
	if(reset_vector < 0x8002000 || reset_vector >= 0x8020000) {
		return false;
	}
	
	// No reason to enter bootloader.
	return true;
}

int main() {
	RCC.enable(RCC.GPIOA);
	RCC.enable(RCC.GPIOB);
	RCC.enable(RCC.GPIOC);
	
	button_inputs.set_mode(Pin::Input);
	button_inputs.set_pull(Pin::PullUp);
	
	button_leds.set_mode(Pin::Output);
	
	led1.set_mode(Pin::Output);
	led2.set_mode(Pin::Output);
	
	if(normal_boot()) {
		chainload(0x8002000);
	}
	
	rcc_init();
	
	// Initialize system timer.
	STK.LOAD = 72000000 / 8 / 1000; // 1000 Hz.
	STK.CTRL = 0x03;
	
	usb_dm.set_mode(Pin::AF);
	usb_dm.set_af(14);
	usb_dp.set_mode(Pin::AF);
	usb_dp.set_af(14);
	
	RCC.enable(RCC.USB);
	
	usb.init();
	
	usb_pu.set_mode(Pin::Output);
	usb_pu.on();
	
	
	while(1) {
		usb.process();
		
		if(do_reset) {
			Time::sleep(10);
			reset();
		}
		
		GPIOC[0].set(Time::time() & 512);
	}
}
