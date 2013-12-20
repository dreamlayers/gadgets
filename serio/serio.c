/* Cross-platform code for the serio serial port library. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

//#define SERIO_ERRORS_FATAL
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <time.h>

#define IN_SERIO
#include "serio.h"

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
