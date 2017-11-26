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

void render_close(void)
{
    rgb_close();
}
