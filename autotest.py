#!/usr/bin/env python

import usb.core
import usb.util
import time, struct

from hidapi import hidapi
import ctypes

from elftools.elf.elffile import ELFFile

pid_runtime = 0x6080
pid_bootloader = 0x6084

e = ELFFile(open('arcin.elf'))

firmware = ''

for segment in sorted(e.iter_segments(), key = lambda x: x.header.p_paddr):
	if segment.header.p_type != 'PT_LOAD':
		continue
	
	data = segment.data()
	lma = segment.header.p_paddr
	
	# Workaround for LD aligning segments to a larger boundary than 8k.
	if lma == 0x8000000:
		lma += 0x2000
		data = data[0x2000:]
	
	# Add padding if necessary.
	firmware += '\0' * (lma - 0x8002000 - len(firmware))
	
	firmware += data

# Align to 64B
if len(firmware) & (64 - 1):
	firmware += '\0' * (64 - (len(firmware) & (64 - 1)))

# Find test board.
dev = usb.core.find(idVendor = 0x1234, idProduct = 0x5678)

if not dev:
	print 'Test board not found.'
	exit(1)

def set_buttons(value):
	dev.ctrl_transfer(0xc0, 0xf0, value, 0, 0)

def get_leds():
	return struct.unpack('<I', ''.join(chr(c) for c in dev.ctrl_transfer(0xc0, 0xf1, 0, 0, 4)))[0]

def count_qe(value):
	dev.ctrl_transfer(0xc0, 0xf2, value & 0xffff, 0, 0)

def open_hiddev(pid):
	global hiddev
	
	hidapi.hid_exit()
	hiddev = hidapi.hid_open(0x1d50, pid, None)
	
	if not hiddev: 
		raise RuntimeError('Target not found.')

def flash_board():
	print 'Found bootloader device, starting flashing.'
	
	# Prepare
	if hidapi.hid_send_feature_report(hiddev, ctypes.c_char_p('\x00\x20'), 2) != 2:
		raise RuntimeError('Prepare failed.')
	
	buf = firmware
	
	# Flash
	while buf:
		if hidapi.hid_write(hiddev, ctypes.c_char_p('\x00' + buf[:64]), 65) != 65:
			raise RuntimeError('Writing failed.')
		buf = buf[64:]
	
	# Finish
	if hidapi.hid_send_feature_report(hiddev, ctypes.c_char_p('\x00\x21'), 2) != 2:
		raise RuntimeError('Finish failed.')
	
	print 'Flashing finished, resetting to runtime.'
	
	# Reset
	if hidapi.hid_send_feature_report(hiddev, ctypes.c_char_p('\x00\x11'), 2) != 2:
		raise RuntimeError('Reset failed.')

class TestFail(Exception):
	pass

def test_leds(value):
	if hidapi.hid_write(hiddev, ctypes.c_char_p('\x00' + struct.pack('<H', value)), 3) != 3:
		raise RuntimeError('Writing failed.')
	
	v = get_leds()
	
	#print 'Set leds to %#x, got back %#x.' % (value, v)
	
	if v != value | 0x30000:
		raise TestFail('leds')

def test_buttons(value):
	set_buttons(value)
	
	time.sleep(0.04)
	
	data = ctypes.create_string_buffer(4)
	if hidapi.hid_read(hiddev, data, 4) != 4:
		raise RuntimeError('Reading failed.')
	
	v = struct.unpack('<HBB', data)[0]
	
	#print 'Set buttons to %#x, got back %#x.' % (value, v)
	
	if v != value:
		raise TestFail('buttons')

def test_qe(value):
	data = ctypes.create_string_buffer(4)
	if hidapi.hid_read(hiddev, data, 4) != 4:
		raise RuntimeError('Reading failed.')
	
	but, a, b = struct.unpack('<HBB', data)
	
	count_qe(value)
	
	time.sleep(0.1)
	
	data = ctypes.create_string_buffer(4)
	if hidapi.hid_read(hiddev, data, 4) != 4:
		raise RuntimeError('Reading failed.')
	
	but, va, vb = struct.unpack('<HBB', data)
	
	#print 'Count of %d: %d -> %d, %d -> %d' % (value, a, va, b, vb)
	
	if (a + value) & 0xff != va:
		raise TestFail('qe1')
	
	if (b + value) & 0xff != vb:
		raise TestFail('qe2')

def test_all():
	print 'Testing leds.'
	
	test_leds(0)
	test_leds(0x7ff)
	
	for i in range(11):
		test_leds(1 << i)
	
	print 'Testing buttons.'
	
	test_buttons(0)
	test_buttons(0x7ff)
	
	for i in range(11):
		test_buttons(1 << i)
	
	print 'Testing encoders.'
	
	test_qe(5)
	test_qe(5)
	test_qe(-5)
	test_qe(-5)
	
	print 'All passed.'

def process():
	try:
		open_hiddev(pid_runtime)
		
		print 'Found runtime device, resetting to bootloader.'
		
		# Reset bootloader
		if hidapi.hid_send_feature_report(hiddev, ctypes.c_char_p('\x00\x10'), 2) != 2:
			raise RuntimeError('Reset failed.')
		
		time.sleep(1)
	except:
		pass
	
	try:
		open_hiddev(pid_bootloader)
		
		flash_board()
		
		time.sleep(1)
		
		open_hiddev(pid_runtime)
		
		test_all()
	
	except TestFail, e:
		print 'Test failed:', e
	
	except RuntimeError, e:
		print 'Error:', e

while 1:
	raw_input('Press enter to start\n')
	
	process()
	print
