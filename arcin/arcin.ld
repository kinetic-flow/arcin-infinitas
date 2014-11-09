MEMORY {
	flash (rx) : org = 0x08002000, len = 120k
	ram (rwx)  : org = 0x20000000, len = 32k
	ccm (rwx)  : org = 0x10000000, len = 8k
}

INCLUDE "arm_flash_ram.ld"
