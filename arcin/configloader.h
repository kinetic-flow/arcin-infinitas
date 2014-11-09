#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <rcc/flash.h>
#include <string.h>

class Configloader {
	private:
		enum {
			MAGIC = 0xc0ff600d,
		};
		
		struct header_t {
			uint32_t magic;
			uint32_t size;
		};
		
		uint32_t flash_addr;
		
	public:
		Configloader(uint32_t addr) : flash_addr(addr) {}
		
		bool read(uint32_t size, void* data) {
			header_t* header = (header_t*)flash_addr;
			
			if(header->magic != MAGIC) {
				return false;
			}
			
			if(header->size < size) {
				size = header->size;
			}
			
			memcpy(data, (void*)(flash_addr + sizeof(header_t)), size);
			
			return true;
		}
		
		bool write(uint32_t size, void* data) {
			header_t header = {MAGIC, size};
			
			// Unlock flash.
			FLASH.KEYR = 0x45670123;
			FLASH.KEYR = 0xCDEF89AB;
			
			// Erase page.
			FLASH.CR = 1 << 1; // PER
			FLASH.AR = flash_addr;
			FLASH.CR = (1 << 6) | (1 << 1); // STRT, PER
			
			while(FLASH.SR & (1 << 0)); // BSY
			
			FLASH.SR &= ~(1 << 5); // EOP
			FLASH.CR = 0;
			
			// Write header.
			uint16_t* src = (uint16_t*)&header;
			uint16_t* dest = (uint16_t*)flash_addr;
			
			for(uint32_t n = 0; n < sizeof(header); n += 2) {
				FLASH.CR = 1 << 0; // PG
				
				*dest++ = *src++;
				
				while(FLASH.SR & (1 << 0)); // BSY
			}
			
			// Write data.
			src = (uint16_t*)data;
			
			for(uint32_t n = 0; n < size; n += 2) {
				FLASH.CR = 1 << 0; // PG
				
				*dest++ = *src++;
				
				while(FLASH.SR & (1 << 0)); // BSY
			}
			
			// Lock flash.
			FLASH.CR = 1 << 7; // LOCK
			
			return true;
		}
};

#endif
