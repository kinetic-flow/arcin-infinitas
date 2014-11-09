#include <rcc/rcc.h>
#include <gpio/gpio.h>
#include <interrupt/interrupt.h>
#include <timer/timer.h>
#include <os/time.h>
#include <usb/usb.h>
#include <usb/descriptor.h>

auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1234, 0x5678, 0x110, 1, 2, 3, 1);
auto conf_desc = configuration_desc(0, 1, 0, 0xc0, 0);

desc_t dev_desc_p = {sizeof(dev_desc), (void*)&dev_desc};
desc_t conf_desc_p = {sizeof(conf_desc), (void*)&conf_desc};

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

class USB_arcin_test : public USB_class_driver {
	private:
		USB_generic& usb;
		
		uint8_t qe_state;
		
		bool set_buttons(uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			button_inputs.set(~wValue);
			
			usb.write(0, nullptr, 0);
			return true;
		}
		
		bool get_leds(uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			if(wLength != 4) {
				return false;
			}
			
			uint32_t buf = (button_leds.get() ^ 0x7ff) | (led1.get() ? 0 : 0x10000) | (led2.get() ? 0 : 0x20000);
			
			usb.write(0, &buf, 4);
			return true;
		}
		
		void inc_qe() {
			switch(qe_state++ & 3) {
				case 0:
					qe1b.on();
					qe2b.on();
					break;
				
				case 1:
					qe1a.on();
					qe2a.on();
					break;
				
				case 2:
					qe1b.off();
					qe2b.off();
					break;
				
				case 3:
					qe1a.off();
					qe2a.off();
					break;
			}
		}
		
		void dec_qe() {
			switch(qe_state-- & 3) {
				case 0:
					qe1a.on();
					qe2a.on();
					break;
				
				case 1:
					qe1b.off();
					qe2b.off();
					break;
				
				case 2:
					qe1a.off();
					qe2a.off();
					break;
				
				case 3:
					qe1b.on();
					qe2b.on();
					break;
			}
		}
		
		bool count_qe(uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			int8_t n = wValue & 0xff;
			
			while(n != 0) {
				if(n > 0) {
					inc_qe();
					n--;
				} else {
					dec_qe();
					n++;
				}
			}
			
			usb.write(0, nullptr, 0);
			return true;
		}
	
	public:
		USB_arcin_test(USB_generic& usbd) : usb(usbd) {
			usb.register_driver(this);
		}
	
	protected:
		virtual SetupStatus handle_setup(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			if(bmRequestType == 0xc0 && bRequest == 0xf0) {
				return set_buttons(wValue, wIndex, wLength) ? SetupStatus::Ok : SetupStatus::Stall;
			}
			
			if(bmRequestType == 0xc0 && bRequest == 0xf1) {
				return get_leds(wValue, wIndex, wLength) ? SetupStatus::Ok : SetupStatus::Stall;
			}
			
			if(bmRequestType == 0xc0 && bRequest == 0xf2) {
				return count_qe(wValue, wIndex, wLength) ? SetupStatus::Ok : SetupStatus::Stall;
			}
			
			return SetupStatus::Unhandled;
		}
};

USB_arcin_test usb_arcin_test(usb);

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
						desc = u"\u0316arcin test";
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
	
	button_inputs.set(0x7ff);
	button_inputs.set_type(Pin::OpenDrain);
	button_inputs.set_mode(Pin::Output);
	
	button_leds.set_mode(Pin::Input);
	button_leds.set_pull(Pin::PullUp);
	
	led1.set_mode(Pin::Input);
	led1.set_pull(Pin::PullUp);
	
	led2.set_mode(Pin::Input);
	led2.set_pull(Pin::PullUp);
	
	qe1a.set(1);
	qe1a.set_type(Pin::OpenDrain);
	qe1a.set_mode(Pin::Output);
	qe1b.set(1);
	qe1b.set_type(Pin::OpenDrain);
	qe1b.set_mode(Pin::Output);
	
	qe2a.set(1);
	qe2a.set_type(Pin::OpenDrain);
	qe2a.set_mode(Pin::Output);
	qe2b.set(1);
	qe2b.set_type(Pin::OpenDrain);
	qe2b.set_mode(Pin::Output);
	
	while(1) {
		usb.process();
	}
}
