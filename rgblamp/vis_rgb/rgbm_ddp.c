#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rgbm.h"

#define NUMLEDS 72
#define DDP_PORT 4048
#define DDP_ADDR "192.168.1.32"

static const unsigned char header[10] = { 0x41, 0x00, 0x01, 0x01, 0x00,
                                          0x00, 0x00, 0x00, 0x00, NUMLEDS * 3 };
static unsigned char buf[10 + NUMLEDS * 3];
static int s = -1;
static struct sockaddr_in server;

int rgbm_hw_open(void) {
    memcpy(buf, header, sizeof(header));

    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket()");
        return 0;
    }

    server.sin_family      = AF_INET;
    server.sin_port        = htons(DDP_PORT);
    server.sin_addr.s_addr = inet_addr(DDP_ADDR);

    return 1;
}

#ifdef _MSC_VER
static __inline long int lrint (double flt) {
    int intgr;
    _asm {
        fld flt
        fistp intgr
    };
    return intgr;
}
#endif

#ifdef RGBM_AGCUP
static unsigned short rgb_srgb2pwm(double n) {
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
#endif

#ifdef RGBM_AGCUP
int rgbm_hw_srgb(const double *rgb)
#else
int rgbm_hw_pwm(const double *rgb)
#endif
{
    int i;
    unsigned char pwm[3];
    for (i = 0; i < 3; i++) {
#ifdef RGBM_AGCUP
        pwm[i] = rgb_srgb2pwm(rgb[i]);
#else
        pwm[i] = lrint(rgb[i] / (4095.0 / 255));
#endif
    }

    for (i = 0; i < NUMLEDS * 3; i += 3) {
        buf[10 + i + 0] = pwm[0];
        buf[10 + i + 1] = pwm[1];
        buf[10 + i + 2] = pwm[2];
    }

    /* Send the message in buf to the server */
    if (sendto(s, buf, sizeof(buf), 0,
             (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("sendto()");
        return 0;
    }
    return 1;
}

void rgbm_hw_close(void)
{
    if (s != -1) {
        close(s);
        s = -1;
    }
}
