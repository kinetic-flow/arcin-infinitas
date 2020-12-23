#ifndef FASTLED_SHIM_DEFINES_H
#define FASTLED_SHIM_DEFINES_H

#include "time.h"

extern "C" {

unsigned long millis() {
    return 0;
}

unsigned long micros() {
    return 0;
}

void delay(unsigned long ms) {
    return;
}

void yield() {
    return;
}

uint32_t get_millisecond_timer() {
    return Time::time();
}

}

#endif