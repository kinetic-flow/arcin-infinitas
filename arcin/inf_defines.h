#ifndef INFINITAS_DEFINES_H
#define INFINITAS_DEFINES_H

// Pins on Arcin board

#define ARCIN_PIN_BUTTON_1        ((uint16_t)(1 << 0))
#define ARCIN_PIN_BUTTON_2        ((uint16_t)(1 << 1))
#define ARCIN_PIN_BUTTON_3        ((uint16_t)(1 << 2))
#define ARCIN_PIN_BUTTON_4        ((uint16_t)(1 << 3))
#define ARCIN_PIN_BUTTON_5        ((uint16_t)(1 << 4))
#define ARCIN_PIN_BUTTON_6        ((uint16_t)(1 << 5))
#define ARCIN_PIN_BUTTON_7        ((uint16_t)(1 << 6))

#define ARCIN_PIN_BUTTON_WHITE    ((uint16_t)(0x55))
#define ARCIN_PIN_BUTTON_BLACK    ((uint16_t)(0x2A))

#define ARCIN_PIN_BUTTON_ALL      ((uint16_t)(0x7F))

#define ARCIN_PIN_BUTTON_8        ((uint16_t)(1 << 7))
#define ARCIN_PIN_BUTTON_9        ((uint16_t)(1 << 8))

#define ARCIN_PIN_BUTTON_START    ((uint16_t)(1 << 9))
#define ARCIN_PIN_BUTTON_SELECT   ((uint16_t)(1 << 10))

// Remapped values for Windows

#define INFINITAS_BUTTON_1        ARCIN_PIN_BUTTON_1
#define INFINITAS_BUTTON_2        ARCIN_PIN_BUTTON_2
#define INFINITAS_BUTTON_3        ARCIN_PIN_BUTTON_3
#define INFINITAS_BUTTON_4        ARCIN_PIN_BUTTON_4
#define INFINITAS_BUTTON_5        ARCIN_PIN_BUTTON_5
#define INFINITAS_BUTTON_6        ARCIN_PIN_BUTTON_6
#define INFINITAS_BUTTON_7        ARCIN_PIN_BUTTON_7

#define INFINITAS_BUTTON_ALL      ARCIN_PIN_BUTTON_ALL

#define INFINITAS_BUTTON_E1       ((uint16_t)(1 << 8))
#define INFINITAS_BUTTON_E2       ((uint16_t)(1 << 9))
#define INFINITAS_BUTTON_E3       ((uint16_t)(1 << 10))
#define INFINITAS_BUTTON_E4       ((uint16_t)(1 << 11))

#define INFINITAS_EFFECTORS_ALL   (INFINITAS_BUTTON_E1 | INFINITAS_BUTTON_E2 | \
                                   INFINITAS_BUTTON_E3 | INFINITAS_BUTTON_E4)

// These are not part of Infinitas controller, but extra generic buttons used
// for digital TT

#define JOY_BUTTON_13             ((uint16_t)(1 << 12))
#define JOY_BUTTON_14             ((uint16_t)(1 << 13))

// buttons[1-9], start, sel, led1, led2
#define ARCIN_LED_COUNT           13

void set_hid_lights(uint16_t leds);
void schedule_led(uint32_t end_time, uint16_t leds_a, uint16_t leds_b);

#endif