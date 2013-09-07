#include <rcc/rcc.h>
#include <gpio/gpio.h>
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

static Pin led1 = GPIOB[14];

USB_f1 usb(USB, dev_desc_p, conf_desc_p);

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
	
	button_inputs.set_mode(Pin::Input);
	button_inputs.set_pull(Pin::PullUp);
	
	button_leds.set_mode(Pin::Output);
	
	led1.set_mode(Pin::Output);
	led1.on();
	
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
		
		if(Time::time() - last_led_time > 1000) {
			button_leds.set(buttons);
		}
		
		if(usb.ep_ready(1)) {
			report_t report = {buttons, uint8_t(TIM2.CNT), uint8_t(TIM3.CNT)};
			
			usb.write(1, (uint32_t*)&report, sizeof(report));
		}
	}
}
