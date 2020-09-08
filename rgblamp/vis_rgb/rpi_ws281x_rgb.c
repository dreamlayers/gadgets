/* Standalone visualization plugin code, using screen instead of RGB lamp. */
/* Copyright 2017 Boris Gjenero. Released under the MIT license. */

#include <math.h>
#include <stdio.h>
/* stdint.h required for ws2811.h */
#include <stdint.h>
#include "ws2811.h"
#include "rgbm.h"

//#define ARRAY_SIZE(stuff)       (sizeof(stuff) / sizeof(stuff[0]))

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                10
#define DMA                     5
//#define STRIP_TYPE            WS2811_STRIP_RGB		// WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE              WS2811_STRIP_GBR		// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW (NOT SK6812RGB)

#define WIDTH                   60
#define HEIGHT                  1
#define LED_COUNT               (WIDTH * HEIGHT)

static ws2811_t ledstring =
{
    .freq = TARGET_FREQ,
    .dmanum = DMA,
    .channel =
    {
        [0] =
        {
            .gpionum = GPIO_PIN,
            .count = LED_COUNT,
            .invert = 0,
            .brightness = 255,
            .strip_type = STRIP_TYPE,
        },
        [1] =
        {
            .gpionum = 0,
            .count = 0,
            .invert = 0,
            .brightness = 0,
        },
    },
};

int rgbm_hw_open(void) {
    ws2811_return_t ret;
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS) {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        return 0;
    } else {
        return 1;
    }
}

int rgbm_hw_pwm(const double *rgb) {
    unsigned int i;
    uint32_t pix = (lrint(rgb[0]) >> 4) | 
                   ((lrint(rgb[1]) >> 4) << 8) |
                   ((lrint(rgb[2]) >> 4) << 16);
    for (i = 0; i < LED_COUNT; i++) {
        ledstring.channel[0].leds[i] = pix;
    }
    ws2811_render(&ledstring);
    return 1;
}

void rgbm_hw_close(void) {
    ws2811_fini(&ledstring);
}
