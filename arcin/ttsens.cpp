#include <os/time.h>
#include <timer/timer.h>

#include "ttsens.h"
#include "inf_defines.h"

#define RESISTANCE_LEVELS 10

// [Most resistance]
// [10] 251 / 255
// [9]  247 / 255
// [8]  243 / 255
// [7]  239 / 255
// [6]  235 / 255
// [5]  231 / 255
// [4]  223 / 255
// [3]  215 / 255
// [2]  211 / 255 <= around here is what people usually use on a real cab, +-1
// [1]    0 / 255 <= default
// [Least resistance]

void update_qe1_arr(int8_t sens);

// in-game values (minus one to account for floating point errors)
static const uint8_t resistance_thresholds[RESISTANCE_LEVELS-1] = {
    210,
    214,
    222,
    230,
    234,
    238,
    242,
    246,
    250
};

// QE1 sensitivity set via the config tool
int8_t original_qe1_sens = 0;

// Current QE1 sensitivity in use.
int8_t qe1_sens = 0;

// last value read from HID output report
uint8_t last_resistance = 0;
uint32_t last_tt_hid_update_time = 0;

void tt_sensitivity_init(int8_t new_qe1_sens) {
    original_qe1_sens = new_qe1_sens;
    qe1_sens = new_qe1_sens;
    update_qe1_arr(qe1_sens);
}

int8_t translate_resistance_to_qe1_sens(uint8_t resistance) {
    uint8_t added_resistance = 0;
    for (int i = 0; i < (RESISTANCE_LEVELS-1); i++) {
        if (resistance < resistance_thresholds[i]) {
            break;
        }

        added_resistance += 1;
    }

    if (added_resistance == 0) {
        return original_qe1_sens;
    }

    // If the original sensitivity was zero,
    //     it was 1:1. Add resistance by moving to 1:2, 1:3, etc.
    // If it was negative,
    //     it was 1:N. Add resistance by moving to 1:N+1, 1:N+2, etc.
    // If it was positive,
    //     it was N:1. Add resistance by moving to N-1:1, N-2:2, down to 1:1, and then to 1:N.

    if (original_qe1_sens == 0) {
        // 0 is 1:1, -2 is 1:2, so always add one
        return -(added_resistance + 1);
    } else if (original_qe1_sens < 0) {
        return original_qe1_sens - added_resistance;
    } else {
        int8_t new_sens = original_qe1_sens - added_resistance;
        // N is 1:N. (for N>1)
        // 1 is 1:1.
        // convert 0 and below to 1:N ratios (0 becomes 1:2, -1 becomes 1:3, etc)
        if (new_sens <= 0) {
            new_sens = new_sens - 2;
        }
        return new_sens;
    }
}

void set_hid_resistance(uint8_t resistance) {
    last_tt_hid_update_time = Time::time();
    if (last_resistance != resistance) {
        last_resistance = resistance;
        qe1_sens = translate_resistance_to_qe1_sens(resistance);
        update_qe1_arr(qe1_sens);
    }
}

void update_qe1_arr(int8_t sens) {
    if (sens < 0) {
        TIM2.ARR = 256 * -sens - 1;
    } else {
        TIM2.ARR = 256 - 1;
    }
}

void check_outdated_tt_sensitivity_hid_report() {
    // if we haven't seen HID update for a while, revert to user-defined value
    if (qe1_sens != original_qe1_sens) {
        uint32_t now = Time::time();
        if (now - last_tt_hid_update_time > 5000) {
            qe1_sens = original_qe1_sens;
            update_qe1_arr(qe1_sens);
        }
    }
}

uint32_t apply_qe1_sens_post(uint32_t raw) {
    uint32_t qe1_count = raw;
    if (qe1_sens < 0) {
        qe1_count /= -qe1_sens;
    } else if (qe1_sens > 0) {
        qe1_count *= qe1_sens;
    }

    return qe1_count;
}
