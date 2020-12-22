#ifndef __INC_LED_SYSDEFS_ARM_LIBONLY_H
#define __INC_LED_SYSDEFS_ARM_LIBONLY_H

#define FASTLED_NAMESPACE_BEGIN namespace NSFastLED {
#define FASTLED_NAMESPACE_END }
#define FASTLED_USING_NAMESPACE using namespace NSFastLED;

#define cli() @do not use cli
#define sei() @do not use sei   

#define FASTLED_ARM

#ifndef INTERRUPT_THRESHOLD
#define INTERRUPT_THRESHOLD 1
#endif

#ifndef FASTLED_ALLOW_INTERRUPTS
#define FASTLED_ALLOW_INTERRUPTS 0
#endif

// pgmspace definitions
#define PROGMEM
#define pgm_read_dword(addr) @no progmem
#define pgm_read_dword_near(addr) @no progmem

#ifndef FASTLED_USE_PROGMEM
#define FASTLED_USE_PROGMEM 0
#endif

// data type defs
typedef volatile       uint8_t RoReg; /**< Read only 8-bit register (volatile const unsigned int) */
typedef volatile       uint8_t RwReg; /**< Read-Write 8-bit register (volatile unsigned int) */

#define FASTLED_NO_PINMAP

#ifndef F_CPU
#define F_CPU 0
#endif

extern "C" {

unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void yield();   

}

#endif
