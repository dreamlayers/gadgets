#include <mqueue.h>
#include "../librgblamp/librgblamp.h"
#include "ws281xproto.h"

static mqd_t mq = (mqd_t)-1;

static struct rgbmsg msg;

RGBAPI bool rgb_open(const char *fn) {
    mq = mq_open("/ws281xproxy", O_WRONLY);
    return mq != (mqd_t)-1;
}

RGBAPI bool rgb_pwm_srgb(double r, double g, double b) {
    msg.type = WSCMD_SRGB;
    msg.rgb[0] = r;
    msg.rgb[1] = g;
    msg.rgb[2] = b;
    return mq_send(mq, (char *)&msg, sizeof(msg), 0) == 0;
}

#if 0
int main(void)
{
    printf("OPN: %i\n", rgb_open("foo"));
    perror("FOO");
    printf("SEND: %i\n", rgb_pwm_srgb(1, 0, 0));
    return 0;
}
#endif

RGBAPI bool rgb_pwm(unsigned short r, unsigned short g, unsigned short b)
{
    msg.type = WSCMD_PWM;
    msg.pwm[0] = r;
    msg.pwm[1] = g;
    msg.pwm[2] = b;
    return mq_send(mq, (char *)&msg, sizeof(msg), 0) == 0;
    return true;
}

RGBAPI bool rgb_matchpwm(unsigned short r, unsigned short g, unsigned short b)
{
    /* TODO */
    return true;
}

RGBAPI bool rgb_flush(void)
{
    /* TODO */
    return true;
}

RGBAPI void rgb_close(void)
{
    /* TODO */
}
