#include <stdio.h>
#include <math.h>
/* stdint.h required for ws2811.h */
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include "coloranim.h"
#include "ws2811.h"

/* GPIO for switching power to LED strip power supply */
#define POWER_GPIO 24

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

void render_open()
{
    ws2811_return_t ret;
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        fatal("failed to open device");
    }

    /* Power supply switching via GPIO */
    wiringPiSetupGpio();
    pinMode(POWER_GPIO, OUTPUT);
    if (digitalRead(POWER_GPIO) == 0) {
        digitalWrite(POWER_GPIO, 1);
        usleep(1000000);
    }
}

static unsigned short srgb2pwm(double n) {
    double r;
    int ir;
    if (n <= 0.04045) {
        r = n / 12.92;
    } else {
        r = pow((n+0.055)/1.055, 2.4);
    }
    ir = (int)lrint(r * 255.0);
    if (ir < 0) ir = 0;
    if (ir > 4095) ir = 255;
    return (unsigned short)ir;
}


void render(const pixel pix)
{
    unsigned int i;
    for (i = 0; i < LED_COUNT; i++) {
        unsigned int pixidx = i * 3;
        ledstring.channel[0].leds[i] =
            srgb2pwm(pix[pixidx]) |
            (srgb2pwm(pix[pixidx + 1]) << 8) |
            (srgb2pwm(pix[pixidx + 2]) << 16);
    }
    ws2811_render(&ledstring);
}

void render_close(void)
{
    int i, allblack = 1;
    for (i = 0; i < LED_COUNT; i++) {
        if (ledstring.channel[0].leds[i] != 0) {
            allblack = 0;
            break;
        }
    }

    ws2811_fini(&ledstring);

    if (allblack) {
        digitalWrite(POWER_GPIO, 0);
    }
}
