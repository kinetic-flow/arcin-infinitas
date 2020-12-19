#ifndef PIXELTYPES_H
#define PIXELTYPES_H

#include <stdint.h>
#include <algorithm>

struct CRGB;
struct CHSV;

/// Forward declaration of hsv2rgb_rainbow here,
/// to avoid circular dependencies.
extern void hsv2rgb_rainbow( const CHSV& hsv, CRGB& rgb);

/// Pre-defined hue values for HSV objects
typedef enum {
    HUE_RED = 0,
    HUE_ORANGE = 32,
    HUE_YELLOW = 64,
    HUE_GREEN = 96,
    HUE_AQUA = 128,
    HUE_BLUE = 160,
    HUE_PURPLE = 192,
    HUE_PINK = 224
} HSVHue;

/// Representation of an HSV pixel (hue, saturation, value (aka brightness)).
struct CHSV {
    union {
		struct {
		    union {
		        uint8_t hue;
		        uint8_t h; };
		    union {
		        uint8_t saturation;
		        uint8_t sat;
		        uint8_t s; };
		    union {
		        uint8_t value;
		        uint8_t val;
		        uint8_t v; };
		};
		uint8_t raw[3];
	};

    /// Array access operator to index into the chsv object
	inline uint8_t& operator[] (uint8_t x) __attribute__((always_inline))
    {
        return raw[x];
    }

    /// Array access operator to index into the chsv object
    inline const uint8_t& operator[] (uint8_t x) const __attribute__((always_inline))
    {
        return raw[x];
    }

    /// default values are UNITIALIZED
    inline CHSV() __attribute__((always_inline))
    {
    }

    /// allow construction from H, S, V
    inline CHSV( uint8_t ih, uint8_t is, uint8_t iv) __attribute__((always_inline))
        : h(ih), s(is), v(iv)
    {
    }

    /// allow copy construction
    inline CHSV(const CHSV& rhs) __attribute__((always_inline))
    {
        h = rhs.h;
        s = rhs.s;
        v = rhs.v;
    }

    inline CHSV& operator= (const CHSV& rhs) __attribute__((always_inline))
    {
        h = rhs.h;
        s = rhs.s;
        v = rhs.v;
        return *this;
    }

    inline CHSV& setHSV(uint8_t ih, uint8_t is, uint8_t iv) __attribute__((always_inline))
    {
        h = ih;
        s = is;
        v = iv;
        return *this;
    }
};

struct CRGB {
	union {
		struct {
            union {
                uint8_t r;
                uint8_t red;
            };
            union {
                uint8_t g;
                uint8_t green;
            };
            union {
                uint8_t b;
                uint8_t blue;
            };
        };
		uint8_t raw[3];
	};

    // default values are UNINITIALIZED
	inline CRGB() __attribute__((always_inline))
    {
    }

    /// allow construction from R, G, B
    inline CRGB( uint8_t ir, uint8_t ig, uint8_t ib) __attribute__((always_inline))
        : r(ir), g(ig), b(ib)
    {
    }

    /// allow construction from HSV color
	inline CRGB(const CHSV& rhs) __attribute__((always_inline))
    {
        hsv2rgb_rainbow( rhs, *this);
    }

    /// allow assignment from one RGB struct to another
	inline CRGB& operator= (const CRGB& rhs) __attribute__((always_inline))
    {
        r = rhs.r;
        g = rhs.g;
        b = rhs.b;
        return *this;
    }

    /// allow assignment from HSV color
	inline CRGB& operator= (const CHSV& rhs) __attribute__((always_inline))
    {
        hsv2rgb_rainbow( rhs, *this);
        return *this;
    }

    /// add one RGB to another, saturating at 0xFF for each channel
    inline CRGB& operator+= (const CRGB& rhs )
    {
        r = r + rhs.r > 0xFF ? 0xFF : r + rhs.r;
        g = g + rhs.g > 0xFF ? 0xFF : g + rhs.g;
        b = b + rhs.b > 0xFF ? 0xFF : b + rhs.b;
        return *this;
    }
};

#endif
