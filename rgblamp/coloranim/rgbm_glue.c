#include <math.h>
#define RGBM_FFTW
#include "rgbm.h"
#include "coloranim.h"

int effect_rgbm(const char *data)
{
    rgbm_run(data);
    return 0;
}


int rgbm_hw_open(void) {
    return 1;
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

int rgbm_hw_pwm(const double *rgb) {
    static double ledstring[PIXCNT * COLORCNT];
    unsigned int i;
    double sr, sg, sb;

    sr = pwm2srgb(rgb[0]);
    sg = pwm2srgb(rgb[1]);
    sb = pwm2srgb(rgb[2]);
    for (i = 0; i < PIXCNT * COLORCNT; i += 3) {
        ledstring[i] = sr;
        ledstring[i+1] = sg;
        ledstring[i+2] = sb;
    }
    render((pixel)&ledstring);
    return 1;
}


void rgbm_hw_close(void) {
}

int rgbm_pollquit(void)
{
    return cmd_cb_pollquit();
}
