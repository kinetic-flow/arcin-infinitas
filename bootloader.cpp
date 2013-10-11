#include <rcc/rcc.h>
#include <rcc/flash.h>
#include <gpio/gpio.h>
#include <interrupt/interrupt.h>
#include <os/time.h>
#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/hid.h>
#include <usb/dfu.h>

void reset() {
	SCB.AIRCR = (0x5fa << 16) | (1 << 2); // SYSRESETREQ
}

auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1d50, 0x6084, 0, 0, 0, 0, 1);
auto conf_desc = configuration_desc(1, 1, 0, 0xc0, 0,
	interface_desc(0, 0, 0, 0xfe, 0x01, 0x02, 0,
		dfu_functional_desc(0x0d, 0, 64, 0x110)
	)
	// HID interface.
	//interface_desc(1, 0, 1, 0x03, 0x00, 0x00, 0,
	//	hid_desc(0x111, 0, 1, 0x22, sizeof(report_desc)),
	//	endpoint_desc(0x81, 0x03, 16, 1)
	//)
);

desc_t dev_desc_p = {sizeof(dev_desc), (void*)&dev_desc};
desc_t conf_desc_p = {sizeof(conf_desc), (void*)&conf_desc};
//desc_t report_desc_p = {sizeof(report_desc), (void*)&report_desc};

static Pin usb_dm = GPIOA[11];
static Pin usb_dp = GPIOA[12];
static Pin usb_pu = GPIOA[15];

static PinArray button_inputs = GPIOB.array(0, 10);
static PinArray button_leds = GPIOC.array(0, 10);

static Pin led1 = GPIOB[14];

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

class USB_DFU : public USB_class_driver {
	private:
		USB_generic& usb;
		
		uint8_t state;
		
		bool get_status(uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			if(wLength > 6) {
				wLength = 6;
			}
			
			uint8_t buf[] = {0, 0, 0, 0, state, 0};
			
			usb.write(0, (uint32_t*)buf, wLength);
			
			return true;
		}
		
		bool download(uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			if(!wLength) {
				state = 2;
				
				if(!flashloader.finish()) {
					return false;
				}
				
				usb.write(0, nullptr, 0);
				
				return true;
			}
			
			if(state == 2) {
				state = 5;
				
				return flashloader.prepare();
			}
			
			return true;
		}
		
	public:
		USB_DFU(USB_generic& usbd) : usb(usbd), state(2) {
			usb.register_driver(this);
		}
	
	protected:
		virtual SetupStatus handle_setup(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			// DFU_GETSTATUS
			if(bmRequestType == 0xa1 && bRequest == 0x03) {
				return get_status(wValue, wIndex, wLength) ? SetupStatus::Ok : SetupStatus::Stall;
			}
			
			// DFU_CLRSTATUS
			// DFU_GETSTATE
			// DFU_ABORT
			
			// DFU_DNLOAD
			if(bmRequestType == 0x21 && bRequest == 0x01) {
				return download(wValue, wIndex, wLength) ? SetupStatus::Ok : SetupStatus::Stall;
			}
			
			return SetupStatus::Unhandled;
		}
		
		virtual void handle_out(uint8_t ep, uint32_t len) {
			if(ep != 0 || len == 0) {
				return;
			}
			
			uint32_t buf[16];
			usb.read(ep, buf, len);
			
			if(state == 5) {
				flashloader.write_block(len, buf);
			}
			
			usb.write(0, nullptr, 0);
		}
};

USB_DFU usb_dfu(usb);

/*
uint32_t last_led_time;

class HID_arcin : public USB_HID {
	public:
		HID_arcin(USB_generic& usbd, desc_t rdesc) : USB_HID(usbd, rdesc, 1, 1, 64) {}
	
	protected:
		virtual bool set_output_report(uint32_t* buf, uint32_t len) {
			last_led_time = Time::time();
			button_leds.set(*buf);
			return true;
		}
};

HID_arcin usb_hid(usb, report_desc_p);
*/

void chainload(uint32_t offset) {
	SCB.VTOR = offset;
	
	asm volatile("ldr sp, [%0]; ldr %0, [%0, #4]; bx %0" :: "r" (offset));
}

uint32_t& reset_reason = *(uint32_t*)0x10000000;
uint32_t* firmware_vtors = (uint32_t*)0x8002000;

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
		
		if(~button_inputs.get() & (1 << 5)) {
			reset();
		}
		
		GPIOC[0].set(Time::time() & 512);
		
		/*
		usb.process();
		
		uint16_t buttons = button_inputs.get() ^ 0x7ff;
		
		if(Time::time() - last_led_time > 1000) {
			button_leds.set(buttons);
		}
		
		if(usb.ep_ready(1)) {
			report_t report = {buttons, uint8_t(TIM2.CNT), uint8_t(TIM3.CNT)};
			
			usb.write(1, (uint32_t*)&report, sizeof(report));
		}
		*/
	}
}
