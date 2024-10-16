#include <stdio.h>
#include <math.h>
#include <string.h>
#include "coloranim.h"
#include "serio.h"

/* Place where pixels are saved */
#define SAVED_PIX  "/dev/shm/arduws_state"

static const unsigned char padding[9];
static unsigned char ledrgb[PIXCNT * 3];

static void render_load(void)
{
    FILE *f = fopen(SAVED_PIX, "rb");
    if (f == NULL || fread(ledrgb, 1, sizeof(ledrgb), f) != sizeof(ledrgb)) {
        memset(ledrgb, 0, sizeof(ledrgb));
    }
    if (f != NULL) fclose(f);
}

void render_open()
{
    if (serio_connect("/dev/ttyACM0", 115200) != SERIO_OK)
        fatal("Failed to open serial port");

    render_load();
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

    for (i = 0; i < PIXCNT * 3; i += 3) {
        ledrgb[i] = srgb2pwm(pix[i + 2]);
        ledrgb[i + 1] = srgb2pwm(pix[i + 1]);
        ledrgb[i + 2] = srgb2pwm(pix[i]);
    }

    serio_putc(8);
    serio_write(ledrgb, sizeof(ledrgb));
    serio_flush();
    serio_write(padding, sizeof(padding));
}

static void render_save(void)
{
    FILE *f;

    f = fopen(SAVED_PIX, "wb");
    if (f == NULL) return;

    fwrite(ledrgb, 1, sizeof(ledrgb), f);
    fclose(f);
}

void render_close(void)
{
    render_save();
    serio_disconnect();
}

void render_get(pixel pix)
{
    unsigned int i;
    for (i = 0; i < PIXCNT * 3; i += 3) {
        pix[i] = pwm2srgb(ledrgb[i + 2]);
        pix[i + 1] = pwm2srgb(ledrgb[i + 1]);
        pix[i + 2] = pwm2srgb(ledrgb[i]);
    }
}

void render_get_avg(pixel pix)
{
    double r = 0, g = 0, b = 0;
    unsigned int i;

    for (i = 0; i < PIXCNT * 3; i += 3) {
        b += pwm2srgb(ledrgb[i]);
        g += pwm2srgb(ledrgb[i + 1]);
        r += pwm2srgb(ledrgb[i + 2]);
    }

    pix[0] = r / PIXCNT;
    pix[1] = g / PIXCNT;
    pix[2] = b / PIXCNT;
}
