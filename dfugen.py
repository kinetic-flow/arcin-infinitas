#!/usr/bin/env python

import sys, struct, zlib
from elftools.elf.elffile import ELFFile

infile = sys.argv[1]
outfile = sys.argv[2]

e = ELFFile(open(infile))

buf = ''

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
	buf += '\0' * (lma - 0x8002000 - len(buf))
	
	buf += data

# Align to 64B
if len(buf) & (64 - 1):
	buf += '\0' * (64 - (len(buf) & (64 - 1)))

# Add DFU suffix
buf += struct.pack('<HHHH3sB',
	0xffff,
	0x5679,
	0x1234,
	0x0100,
	'UFD',
	16,
)

# Calculate CRC.
buf += struct.pack('<I', ~zlib.crc32(buf) & 0xffffffff)

open(outfile, 'w').write(buf)
