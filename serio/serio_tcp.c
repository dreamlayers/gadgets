/* Unix code for the serio serial port library. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <termios.h>

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <ctype.h>

#include "../common/tcp.h"
#include <netinet/tcp.h>

#define IN_SERIO
#include "serio.h"

/* Clock to use for timeouts */
#ifdef _POSIX_MONOTONIC_CLOCK
#define TIMEOUT_CLOCK CLOCK_MONOTONIC
#else
#define TIMEOUT_CLOCK CLOCK_REALTIME
#endif

/*** PORT CONTROL ***/

SERIO_RETURN serio_purge(unsigned int what) {
#if 0    
    int flags = 0;

    if (what & SERIO_PURGEIN) {
        if (what & SERIO_PURGEOUT) {
        flags = TCIOFLUSH;
        } else {
            flags = TCIFLUSH;
        }
    } else if (what & SERIO_PURGEOUT) {
        flags = TCOFLUSH;
    }

    if (tcflush(fd, flags) != 0) {
        SERIO_ERROR("serio_purge: tcflush failed");
    } else {
        SERIO_SUCCESS();
    }
#endif    
    SERIO_SUCCESS();
}


SERIO_RETURN serio_flush(void) {
    if (tcdrain(fd) != 0) {
        SERIO_ERROR("serio_flush: tcdrain failed");
    }
    SERIO_SUCCESS();
}

/*** SETUP AND INITIALIZATION ***/

SERIO_RETURN serio_connect_tcp(const char *address, unsigned int port) {
    int size = 0;
    int flag = 1;

    fd = tcpc_open(address, port);
    if (fd == -1) {
        SERIO_ERROR("serio_connect_tcp: tcpc_open failed");
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) != 0) {
        SERIO_ERROR("serio_connect_tcp: TCP_NODELAY failed");
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF,  &size, sizeof(int)) != 0) {
        SERIO_ERROR("serio_connect_tcp: SO_SNDBUF setting failed");
    }

    serio_tcp = 1;
    SERIO_SUCCESS();
}

void serio_disconnect_tcp(void) {
    tcpc_close(fd);
    fd = -1;
}
