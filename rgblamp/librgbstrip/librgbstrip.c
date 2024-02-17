#include <math.h>
#include "serio.h"
#define BUILDING_LIBRGBLAMP
#include "librgblamp.h"

#ifdef _MSC_VER
static __inline long int lrint (double flt) {
    int intgr;
    _asm {
        fld flt
        fistp intgr
    };
    return intgr;
}
#endif

static unsigned short rgb_srgb2pwm(double n) {
    double r;
    int ir;
    if (n <= 0.04045) {
        r = n / 12.92;
    } else {
        r = pow((n+0.055)/1.055, 2.4);
    }
    ir = (int)lrint(r * 255.0);
    if (ir < 0) ir = 0;
    if (ir > 255) ir = 255;
    return (unsigned short)ir;
}

static unsigned short rgb_srgb256pwm(int n) {
    return rgb_srgb2pwm((double)n / 255.0);
}

#ifdef WIN32
#define RGBPORT "COM3"
#else
#define RGBPORT "/usr/local/etc/rgbstrip"
#endif

RGBAPI bool rgb_open(const char *fn) {
    if (serio_connect(RGBPORT /* FIXME */, 115200) < 0) return false;
    return true;
}

RGBAPI void rgb_close(void) {
    serio_disconnect();
}

RGBAPI bool rgb_getout_srgb256(unsigned char *dest) {
    /* Not implemented in the firmware. Return fully saturated colour,
     * so gradient rectangle in winrgbchoose works best.
     */
    dest[0] = 0;
    dest[1] = 255;
    dest[2] = 0;
    return true;
}

static unsigned char rgb_pwm12to8(unsigned short n) {
    return (n > 4095) ? 255 : (n >> 4);
}

static bool rgb_sendpwm(unsigned char r,
                        unsigned char g,
                        unsigned char b) {
    unsigned char buf[4];
    buf[0] = 1;
    buf[1] = b;
    buf[2] = g;
    buf[3] = r;
    return serio_write(buf, sizeof(buf)) >= 0;
}

RGBAPI bool rgb_pwm(unsigned short r,
                    unsigned short g,
                    unsigned short b) {
    return rgb_sendpwm(rgb_pwm12to8(r),
                       rgb_pwm12to8(g),
                       rgb_pwm12to8(b));
}

RGBAPI bool rgb_pwm_srgb(double r, double g, double b) {
    return rgb_sendpwm(rgb_srgb2pwm(r),
                       rgb_srgb2pwm(g),
                       rgb_srgb2pwm(b));
}

RGBAPI bool rgb_pwm_srgb256(unsigned char r,
                            unsigned char g,
                            unsigned char b) {
    return rgb_sendpwm(rgb_srgb256pwm(r),
                       rgb_srgb256pwm(g),
                       rgb_srgb256pwm(b));
}

RGBAPI bool rgb_matchpwm(unsigned short r,
                         unsigned short g,
                         unsigned short b) {
    /* TODO: The firmware doesn't support transitions,
     *       so preparing for them is not necessary.
     */
    return true;
}

RGBAPI bool rgb_matchpwm_srgb(double r, double g, double b) {
    return rgb_matchpwm(rgb_srgb2pwm(r),
                        rgb_srgb2pwm(g),
                        rgb_srgb2pwm(b));
}

RGBAPI bool rgb_matchpwm_srgb256(unsigned char r,
                                 unsigned char g,
                                 unsigned char b) {
    return rgb_matchpwm_srgb(rgb_srgb256pwm(r),
                             rgb_srgb256pwm(g),
                             rgb_srgb256pwm(b));
}
RGBAPI bool rgb_flush(void) {
    if (serio_flush() == SERIO_OK) {
        return true;
    } else {
        return false;
    }
}
