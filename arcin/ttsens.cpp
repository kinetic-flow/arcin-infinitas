#include <os/time.h>
#include <timer/timer.h>

#include "ttsens.h"
#include "inf_defines.h"

// [Most resistance]
// [ingame value] / [percentage] / [out of 255]
// 9 = 98% = 250
// 8 = 97% = 247
// 7 = 95% = 242
// 6 = 94% = 240
// 5 = 92% = 234
// 4 = 91% = 232
// 3 = 87% = 221
// 2 = 84% = 214
// 1 = 83% = 211
// 0 =  0% =   0
// [Least resistance]

void update_qe1_arr(int8_t sens);
void check_outdated_hid_report();

static const uint8_t resistance_thresholds[] = {
    210,
    213,
    218,
    227,
    233,
    237,
    241,
    245,
    249
};

static const int8_t sensitivity_ratios_negative[10] = {
    0,   // 1:1
    -2,  // 1:2
    -3,  // 1:3
    -4,
    -5,
    -6,
    -7,
    -8,
    -11,
    -16  // 1:16
};

static const int8_t sensitivity_ratios_positive[10] = {
    16,  // 16:1
    11,
    8,
    7,
    6,
    5,
    4,   // 4:1
    3,   // 3:1
    2,   // 2:1
    0    // 1:1
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
    for (int i = 0; i < 9; i++) {
        if (resistance <= resistance_thresholds[i]) {
            break;
        }

        added_resistance += 1;
    }

    if (original_qe1_sens <= 0) {
        return sensitivity_ratios_negative[added_resistance];
    } else {
        return sensitivity_ratios_positive[added_resistance];
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

void check_outdated_hid_report() {
    // if we haven't seen HID update for a while, revert to user-defined value
    if (qe1_sens != original_qe1_sens) {
        uint32_t now = Time::time();
        if (now - last_tt_hid_update_time > 10000) {
            qe1_sens = original_qe1_sens;
            update_qe1_arr(qe1_sens);
        }
    }
}

uint32_t apply_qe1_sens_post(uint32_t raw) {
    check_outdated_hid_report();

    uint32_t qe1_count = raw;
    if (qe1_sens < 0) {
        qe1_count /= -qe1_sens;
    } else if (qe1_sens > 0) {
        qe1_count *= qe1_sens;
    }

    return qe1_count;
}
