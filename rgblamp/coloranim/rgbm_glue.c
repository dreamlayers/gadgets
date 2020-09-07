#include <math.h>
#define RGBM_FFTW
#include "rgbm.h"
#include "coloranim.h"
#include "librgblamp.h"

//extern void rgbm_run(const char *snddev);
int effect_rgbm(const char *data)
{
    rgbm_run(data);
    return 0;
}


RGBAPI bool rgb_open(const char *fn) {
    return true;
}

static double pwm2srgb(unsigned short n) {
    double r = n / 4095.0;

    if (r < 0.0) r = 0.0;
    if (r > 1.0) r = 1.0;

    if (r <= 0.0031308) {
        r = 12.92 * r;
    } else {
        r = 1.055 * pow(r, 1/2.4) - 0.055;
    }
    if (r > 1.0) r = 1.0;
    if (r < 0.0) r = 0.0;
    return r;
}


RGBAPI bool rgb_pwm(unsigned short r, unsigned short g, unsigned short b) {
    static double ledstring[PIXCNT * COLORCNT];
    unsigned int i;
    double sr, sg, sb;

    sr = pwm2srgb(r);
    sg = pwm2srgb(g);
    sb = pwm2srgb(b);
    for (i = 0; i < PIXCNT * COLORCNT; i += 3) {
        ledstring[i] = sr;
        ledstring[i+1] = sg;
        ledstring[i+2] = sb;
    }
    render((pixel)&ledstring);
    return true;
}

RGBAPI bool rgb_flush(void) {
    return true;
}

RGBAPI bool rgb_matchpwm(unsigned short r, unsigned short g, unsigned short b) {
    (void)r;
    (void)g;
    (void)b;
    return true;
}

RGBAPI void rgb_close(void) {
}

int rgbm_pollquit(void)
{
    return cmd_cb_pollquit();
}
