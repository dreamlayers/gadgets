#include <stdio.h>
#include <math.h>
/* stdint.h required for ws2811.h */
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include "coloranim.h"
#include "ws2811.h"

/* Place where pixels are saved */
#define SAVED_PIX  "/dev/shm/coloranim_state"
/* GPIO for switching power to LED strip power supply */
#define POWER_GPIO 24

// defaults for cmdline options
#define TARGET_FREQ             WS2811_TARGET_FREQ
#define GPIO_PIN                10
// Using DMA channel 5 will cause]() filesystem corruption on the RPi 3 B
// https://github.com/jgarff/rpi_ws281x/issues/224
#define DMA                     10
//#define STRIP_TYPE            WS2811_STRIP_RGB		// WS2812/SK6812RGB integrated chip+leds
#define STRIP_TYPE              WS2811_STRIP_GBR		// WS2812/SK6812RGB integrated chip+leds
//#define STRIP_TYPE            SK6812_STRIP_RGBW		// SK6812RGBW (NOT SK6812RGB)

#define WIDTH                   PIXCNT
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

static void render_load(void)
{
    FILE *f;
    unsigned char buf[LED_COUNT * 3], *p;
    unsigned int i;

    f = fopen(SAVED_PIX, "rb");
    if (f != NULL && fread(buf, 1, sizeof(buf), f) == sizeof(buf)) {
        p = buf;
        for (i = 0; i < LED_COUNT; i++) {
            ledstring.channel[0].leds[i] = *p | (*(p+1) << 8) | (*(p+2) << 16);
            p += 3;
        }
    } else {
        for (i = 0; i < LED_COUNT; i++) {
           ledstring.channel[0].leds[i] = 0;
        }
    }
    if (f != NULL) fclose(f);
}

void render_open()
{
    ws2811_return_t ret;
    if ((ret = ws2811_init(&ledstring)) != WS2811_SUCCESS)
    {
        fprintf(stderr, "ws2811_init failed: %s\n", ws2811_get_return_t_str(ret));
        fatal("failed to open device");
    }

    render_load();

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
    if (ir > 255) ir = 255;
    return (unsigned short)ir;
}

static double pwm2srgb(unsigned short n) {
    double r = n / 255.0;
    if (r <= 0.0031308) {
        r = 12.92 * r;
    } else {
        r = 1.055 * pow(r, 1/2.4) - 0.055;
    }
    if (r > 1.0) r = 1.0;
    if (r < 0.0) r = 0.0;
    return r;
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

static void render_save(void)
{
    FILE *f;
    unsigned char buf[LED_COUNT * 3], *p;
    unsigned int i;

    f = fopen(SAVED_PIX, "wb");
    if (f == NULL) return;

    p = buf;
    for (i = 0; i < LED_COUNT; i++) {
        ws2811_led_t led = ledstring.channel[0].leds[i];
        *(p++) = led & 0xFF;
        *(p++) = (led >> 8) & 0xFF;
        *(p++) = (led >> 16) & 0xFF;
    }

    fwrite(buf, 1, sizeof(buf), f);
    fclose(f);
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

    if (!allblack) {
        render_save();
    } else {
        unlink(SAVED_PIX);
    }

    ws2811_fini(&ledstring);

    if (allblack) {
        digitalWrite(POWER_GPIO, 0);
    }
}

void render_get(pixel pix)
{
    unsigned int i;
    pixel pp = pix;;

    pp = pix;
    for (i = 0; i < LED_COUNT; i++) {
        ws2811_led_t led = ledstring.channel[0].leds[i];
        *(pp++) = pwm2srgb(led & 0xFF);
        *(pp++) = pwm2srgb((led >> 8) & 0xFF);
        *(pp++) = pwm2srgb((led >> 16) & 0xFF);
    }

}
