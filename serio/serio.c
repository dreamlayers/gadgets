/* Cross-platform code for the serio serial port library. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

//#define SERIO_ERRORS_FATAL
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <time.h>

#define IN_SERIO
#include "serio.h"

int serio_tcp = 0;
#ifdef SERIO_ABORT_POLL
int (*abortpollf)(void);
#endif

/*** WRITING ***/

SERIO_RETURN serio_putc(unsigned char c) {
#ifndef SERIO_ERRORS_FATAL
    return serio_write(&c, 1);
#else
    serio_write(&c, 1);
#endif
}

/*** READING ***/

int serio_getc(void) {
    unsigned char c;
#if defined(SERIO_STRICT_READS) && defined(SERIO_ERRORS_FATAL)
    serio_read(&c,1);
    return c;
#else
    int res;
    res = (int)serio_read(&c, 1);
    if (res == 1) {
        return c;
    } else if (res == 0) {
        return SERIO_TIMEOUT;
    } else {
        return (int)res;
    }
#endif
}

/*** PORT CONTROL ***/

#ifdef SERIO_ABORT_POLL
void serio_setabortpoll(int (*func)(void)) {
    abortpollf = func;
}
#endif

#ifndef WIN32
SERIO_RETURN serio_connect(const char *name, unsigned int baud) {
    unsigned int port = 0;
    unsigned int mul = 1;
    const char *nameend = name + strlen(name) - 1;
    const char *p = nameend;
    while (p > name && *p >= '0' && *p <= '9' && mul < 100000) {
        port += (*p - '0') * mul;
        mul *= 10;
        p--;
    }
    if (p > name && p < nameend && *p == ':') {
        int adrlen = p - name;
        char *address = malloc(adrlen + 1);
#ifndef SERIO_ERRORS_FATAL
        int ret;
#endif
        memcpy(address, name, adrlen);
        address[adrlen] = 0;
#ifdef SERIO_ERRORS_FATAL
        serio_connect_tcp(address, port);
#else
        ret = serio_connect_tcp(address, port);
        free(address);
        return ret;
#endif
    } else {
        SERIO_PROPAGATE_ERROR(serio_connect_com(name, baud));
    }
}

void serio_disconnect(void) {
    if (serio_tcp == 0) {
        serio_disconnect_com();
    } else {
        serio_disconnect_tcp();
    }
}
#endif

int serio_secskalive(void) {
    return serio_tcp ? 60 : 0;
}
