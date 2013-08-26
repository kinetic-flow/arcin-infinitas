#include <rcc/rcc.h>
#include <gpio/pin.h>
#include <timer/timer.h>
#include <os/time.h>
#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/hid.h>

auto report_desc = gamepad(
	// Inputs.
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
	
	padding_out(5)
);

auto dev_desc = device_desc(0x200, 0, 0, 0, 64, 0x1234, 0x5678, 0, 0, 0, 0, 1);
auto conf_desc = configuration_desc(1, 1, 0, 0xc0, 0,
	// HID interface.
	interface_desc(1, 0, 1, 0x03, 0x00, 0x00, 0,
		hid_desc(0x111, 0, 1, 0x22, sizeof(report_desc)),
		endpoint_desc(0x81, 0x03, 16, 1)
	)
);

desc_t dev_desc_p = {sizeof(dev_desc), (void*)&dev_desc};
desc_t conf_desc_p = {sizeof(conf_desc), (void*)&conf_desc};

Pin& usb_dm = PA11;
Pin& usb_dp = PA12;
Pin& usb_pu = PA15;

USB_f1 usb(USB, dev_desc_p, conf_desc_p);

uint32_t last_led_time;

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
				if(wLength > sizeof(report_desc)) {
					wLength = sizeof(report_desc);
				}
				
				uint32_t* p = (uint32_t*)&report_desc;
				
				while(wLength >= 64) {
					usb.write(0, p, 64);
					p += 64/4;
					wLength -= 64;
					
					while(!usb.ep_ready(0));
				}
				
				usb.write(0, p, wLength);
				return SetupStatus::Ok;
			}
			
			// Set output report.
			if(bmRequestType == 0x21 && bRequest == 0x09 && wValue == 0x0200) {
				//return set_output(wLength) ? SetupStatus::Ok : SetupStatus::Stall;
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
			
			if(len == 2) {
				uint32_t buf;
				usb.read(ep, &buf, len);
				last_led_time = Time::time();
				GPIOC.reg.ODR = buf & 0x7ff;
			}
		}
};

USB_HID usb_hid(usb);

struct report_t {
	uint16_t buttons;
	uint8_t axis_x;
	uint8_t axis_y;
} __attribute__((packed));

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
	
	RCC.enable(RCC.TIM2);
	RCC.enable(RCC.TIM3);
	
	TIM2.CCMR1 = (1 << 8) | (1 << 0);
	TIM2.CCER = 1 << 1;
	TIM2.SMCR = 3;
	TIM2.CR1 = 1;
	
	TIM3.CCMR1 = (1 << 8) | (1 << 0);
	TIM3.CCER = 1 << 1;
	TIM3.SMCR = 3;
	TIM3.CR1 = 1;
	
	PA0.set_af(1);
	PA1.set_af(1);
	PA0.set_mode(Pin::AF);
	PA1.set_mode(Pin::AF);
	
	PA6.set_af(2);
	PA7.set_af(2);
	PA6.set_mode(Pin::AF);
	PA7.set_mode(Pin::AF);
	
	while(1) {
		usb.process();
		
		uint16_t buttons = (~GPIOB.reg.IDR & 0x7ff);
		
		if(Time::time() - last_led_time > 1000) {
			GPIOC.reg.ODR = buttons;
		}
		
		if(usb.ep_ready(1)) {
			report_t report = {buttons, uint8_t(TIM2.CNT), uint8_t(TIM3.CNT)};
			
			usb.write(1, (uint32_t*)&report, sizeof(report));
		}
	}
}
