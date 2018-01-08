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
    msg.type = 0;
    msg.rgb[0] = r;
    msg.rgb[1] = g;
    msg.rgb[2] = b;
    return mq_send(mq, (char *)&msg, sizeof(msg), 0) == 0;
}

int main(void)
{
    printf("OPN: %i\n", rgb_open("foo"));
    perror("FOO");
    printf("SEND: %i\n", rgb_pwm_srgb(1, 0, 0));
    return 0;
}
