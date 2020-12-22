#ifndef FASTLED_SHIM_DEFINES_H
#define FASTLED_SHIM_DEFINES_H

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

}

#endif