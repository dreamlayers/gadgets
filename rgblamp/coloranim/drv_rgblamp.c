#include <string.h>
#include "coloranim.h"
#include <librgblamp.h>

#ifdef WIN32
#define RGBPORT "COM8"
#else
#define RGBPORT "/dev/ttyUSB0"
#endif

void render_open(void)
{
    if (!rgb_open(RGBPORT))
        fatal("connecting to RGB lamp");
}

void render(const pixel pixr)
{
    rgb_pwm_srgb(pixr[0], pixr[1], pixr[2]);
    rgb_flush();
}

/* TODO: Maybe remember last colour, to avoid querying lamp */
void render_get(pixel pix)
{
    rgb_getout_srgb(pix);
}

void render_get_avg(pixel pix)
{
    rgb_getout_srgb(pix);
}

void render_close(void)
{
    double rgb[3];
    if (rgb_getout_srgb(rgb)) {
        rgb_matchpwm_srgb(rgb[0], rgb[1], rgb[2]);
    }
    rgb_close();
}
