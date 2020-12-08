#ifndef REPORT_DESC_H
#define REPORT_DESC_H

#include <usb/hid.h>

#include "usb_strings.h"

constexpr HID_Item<uint8_t> string_index(uint8_t x) {
	return hid_item(0x78, x);
}

constexpr HID_Item<uint8_t> string_range_begin(uint8_t x) {
	return hid_item(0x89, x);
}

constexpr HID_Item<uint8_t> string_range_end(uint8_t x) {
	return hid_item(0x99, x);
}

auto report_desc = gamepad(
	// Inputs.
	report_id(1),
	
	buttons(15),
	padding_in(1),
	
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

	usage_page(UsagePage::Ordinal),
	usage(12),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),

	usage_page(UsagePage::Ordinal),
	usage(13),
	collection(Collection::Logical, 
		usage_page(UsagePage::LED),
		usage(0x4b),
		report_size(1),
		report_count(1),
		output(0x02)
	),
	
	padding_out(3),
	
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

auto keyb_report_desc = keyboard(
	usage_page(UsagePage::Keyboard),
	report_size(1),
	report_count(13),
	logical_minimum(0),
	logical_maximum(255),
	usage_minimum(0),
	usage_maximum(255),
	report_count(13),
	report_size(8),
	input(0x00)
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

#endif
