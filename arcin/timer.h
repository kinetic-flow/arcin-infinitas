#ifndef TIMER_DEFINES_H
#define TIMER_DEFINES_H

#include <stdint.h>
#include <os/time.h>

class timer {
private:
    bool armed = false;
    uint32_t time_to_expire = 0;

public:
    timer() {
        reset();
    }

    void arm(uint32_t milliseconds_from_now) {
        uint32_t now = Time::time();
        time_to_expire = now + milliseconds_from_now;
        armed = true;
    }

    void reset() {
        armed = false;
    }

    bool is_armed() {
        return armed;
    }

    bool is_expired() {
        if (!is_armed()) {
            return false;
        }

        uint32_t now = Time::time();
        int32_t diff = now - time_to_expire;
        return (diff > 0);
    }

    bool check_if_expired_reset() {
        bool expired = is_expired();
        if (expired) {
            reset();
        }
        
        return expired;
    }
};

#endif
