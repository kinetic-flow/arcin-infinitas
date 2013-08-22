#include <rcc/rcc.h>
#include <gpio/pin.h>
#include <os/time.h>
#include <usb/usb.h>
#include <usb/descriptor.h>

template <uint32_t S>
struct raw_t {
	uint8_t a[S];
} __attribute__((packed));

uint8_t report_desc[] = {
	0x05, 0x01, // Usage page: Generic Desktop
	0x09, 0x05, // Usage: Gamepad
	0xa1, 0x01, // Collection: Application
		0xa1, 0x00, // Collection: Physical
			0x05, 0x09, // Usage page: Button
			0x19, 0x01, // Usage minimum: Button 1
			0x29, 0x20, // Usage maximum: Button 32
			0x15, 0x00, // Logical minimum: 0
			0x25, 0x01, // Logical maximum: 1
			0x95, 0x20, // Report count: 32
			0x75, 0x01, // Report size: 1
			0x81, 0x02, // Input (data, var, abs)
		0xc0, // End collection
	0xc0, // End collection
};

raw_t<9> hid_desc = {{0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, sizeof(report_desc), 0x00}};

auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1234, 0x5678, 0, 0, 0, 0, 1);
auto conf_desc = configuration_desc(1, 1, 0, 0xc0, 0,
	// HID interface.
	interface_desc(0, 0, 1, 0x03, 0x00, 0x00, 0,
		hid_desc,
		endpoint_desc(0x81, 0x03, 16, 1)
	)
);

desc_t dev_desc_p = {sizeof(dev_desc), (void*)&dev_desc};
desc_t conf_desc_p = {sizeof(conf_desc), (void*)&conf_desc};

Pin& usb_dm = PA11;
Pin& usb_dp = PA12;
Pin& usb_pu = PA15;

USB_f1 usb(USB, dev_desc_p, conf_desc_p);

class USB_HID : public USB_class_driver {
	private:
		USB_generic& usb;
		
		uint32_t buf[16];
	
	public:
		USB_HID(USB_generic& usbd) : usb(usbd) {
			usb.register_driver(this);
		}
	
	protected:
		virtual SetupStatus handle_setup(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength) {
			// Get report descriptor.
			if(bmRequestType == 0x81 && bRequest == 0x06 && wValue == 0x2200) {
				usb.write(0, (uint32_t*)report_desc, sizeof(report_desc) < wLength ? sizeof(report_desc) : wLength);
				return SetupStatus::Ok;
			}
			
			return SetupStatus::Unhandled;
		}
		
		virtual void handle_set_configuration(uint8_t configuration) {
			if(configuration) {
				//usb.register_out_handler(this, 1);
				usb.hw_conf_ep(0x81, EPType::Interrupt, 16);
			}
		}
		
		virtual void handle_out(uint8_t ep, uint32_t len) {
			if(ep == 0) {
				usb.write(0, nullptr, 0);
			}
		}
};

USB_HID usb_hid(usb);

int main() {
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
	
	PB0.set_pull(Pin::PullUp);
	PB1.set_pull(Pin::PullUp);
	PB2.set_pull(Pin::PullUp);
	PB3.set_pull(Pin::PullUp);
	PB4.set_pull(Pin::PullUp);
	PB5.set_pull(Pin::PullUp);
	PB6.set_pull(Pin::PullUp);
	PB7.set_pull(Pin::PullUp);
	PB8.set_pull(Pin::PullUp);
	PB9.set_pull(Pin::PullUp);
	PB10.set_pull(Pin::PullUp);
	
	PC0.set_mode(Pin::Output);
	PC1.set_mode(Pin::Output);
	PC2.set_mode(Pin::Output);
	PC3.set_mode(Pin::Output);
	PC4.set_mode(Pin::Output);
	PC5.set_mode(Pin::Output);
	PC6.set_mode(Pin::Output);
	PC7.set_mode(Pin::Output);
	PC8.set_mode(Pin::Output);
	PC9.set_mode(Pin::Output);
	PC10.set_mode(Pin::Output);
	
	PB14.set_mode(Pin::Output);
	PB14.on();
	
	while(1) {
		usb.process();
		
		uint32_t buttons = (~GPIOB.reg.IDR & 0x7ff);
		GPIOC.reg.ODR = buttons;
		
		if(usb.ep_ready(1)) {
			usb.write(1, &buttons, sizeof(buttons));
		}
	}
}
