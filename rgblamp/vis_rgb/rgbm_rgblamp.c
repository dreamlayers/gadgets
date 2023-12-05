/* RGB lamp driver for music visualizer. */
/* Copyright 2020 Boris Gjenero. Released under the MIT license. */

#include "rgbm.h"
#include "librgblamp.h"

#ifdef WIN32
#define RGBPORT "COM8"
#else
#define RGBPORT "/usr/local/etc/rgblamp"
#endif

/* Set after successful PWM write, and enables rgb_matchpwm afterwards */
static int wrotepwm;
static double rgbsav[3];

int rgbm_hw_open(void)
{
    wrotepwm = 0;
    return rgb_open(RGBPORT);
}

#ifdef RGBM_AGCUP
int rgbm_hw_srgb(const double *rgb)
#else
int rgbm_hw_pwm(const double *rgb)
#endif
{
    int res;
    rgb_flush();
#ifdef RGBM_AGCUP
    res = rgb_pwm_srgb(rgb[0], rgb[1], rgb[2]);
#else
    res = rgb_pwm(rgb[0], rgb[1], rgb[2]);
#endif
    if (res) {
        wrotepwm = 1;
        rgbsav[0] = rgb[0];
        rgbsav[1] = rgb[1];
        rgbsav[2] = rgb[2];
    }
    return res;
}

void rgbm_hw_close(void)
{
    if (wrotepwm) {
#ifdef RGBM_AGCUP
        rgb_matchpwm_srgb(rgbsav[0], rgbsav[1], rgbsav[2]);
#else
        rgb_matchpwm(rgbsav[0], rgbsav[1], rgbsav[2]);
#endif
    }
    rgb_close();
}
