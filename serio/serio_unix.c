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

#define IN_SERIO
#include "serio.h"

/* Clock to use for timeouts */
#ifdef _POSIX_MONOTONIC_CLOCK
#define TIMEOUT_CLOCK CLOCK_MONOTONIC
#else
#define TIMEOUT_CLOCK CLOCK_REALTIME
#endif

/*** GLOBAL VARIABLES ***/

static int fd;
static int readtmout = 0;

/*** ERROR MANAGEMENT ***/

#if defined(__GNUC__)
static void fatalerr(char *s) __attribute__ ((noreturn));
#endif
static void fatalerr(char *s) {
    fputs(s, stderr);
    exit(-1);
}

/*** TIMEOUT HANDLING ***/


static void timespec_add_ms(struct timespec *ts, unsigned int milliseconds) {
    ts->tv_sec += milliseconds / 1000;
    ts->tv_nsec += (milliseconds % 1000) * 1000000;

    if (ts->tv_nsec > 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}

static long timespec_ms_remains(struct timespec *ts) {
    struct timespec curts;
    long remains;

    clock_gettime(TIMEOUT_CLOCK, &curts);

    remains = ts->tv_nsec - curts.tv_nsec;

    /* Nanoseconds to millseconds, rounding up */
    remains = (remains + 999999) / 1000000;

    remains += (ts->tv_sec - curts.tv_sec) * 1000;

    return remains;
}

/*** WRITING ***/

/* Based on serio_read, but simplifed due to lack of write timeout. */
SERIO_RETURN serio_write(const unsigned char *s, size_t l) {
    int res = -1;
    fd_set fds;
    const unsigned char *p = s;
    size_t remains = l;

    while (1) {
#ifdef SERIO_ABORT_POLL
        struct timeval tv;
#endif

        FD_ZERO(&fds);
        FD_SET(fd, &fds);

#ifdef SERIO_ABORT_POLL
        if (abortpollf != NULL) {
            tv.tv_sec = 0;
            tv.tv_usec = SERIO_ABORT_POLL_MS * 1000;

            res = select(fd+1, NULL, &fds, NULL, &tv);
        } else
#endif /* SERIO_ABORT_POLL */
        res = select(fd+1, NULL, &fds, NULL, NULL);
        if (res == 1) {
            /* select() indicates data is available */
            res = write(fd, p, remains);

            if (res < 0 && errno != EAGAIN
#if EAGAIN != EWOULDBLOCK
                && errno != EWOULDBLOCK
#endif
                ) {
                SERIO_ERROR("serio_write: write failed");
            }

            remains -= res;
            if (remains == 0) {
                /* Wrote requested amount successfully */
                SERIO_SUCCESS();
            }
            p += res;
        }
#ifdef SERIO_ABORT_POLL
        else if (res == 0) {
            /* select() timeout */
            if (abortpollf != NULL && abortpollf()) {
                return SERIO_ABORT;
            }
        }
#endif /* SERIO_ABORT_POLL */
        else {
            SERIO_ERROR("serio_read: select failed");
        }
    } /* while */
}

/*** READING ***/

SERIO_LENGTH_RETURN serio_read(unsigned char *s, size_t l) {
    int res;
    fd_set rfds;
    unsigned char *p = s;
    size_t remains = l;
    struct timeval tv;

    long ms_wait = readtmout;
    struct timespec duetime;

    clock_gettime(TIMEOUT_CLOCK, &duetime);
    timespec_add_ms(&duetime, readtmout);

    while (1) {
#ifdef SERIO_ABORT_POLL
        /* Limit select() timeout to abort poll interval. */
        if (abortpollf && ms_wait > SERIO_ABORT_POLL_MS) {
            ms_wait = SERIO_ABORT_POLL_MS;
        }
#endif
        tv.tv_sec = ms_wait / 1000;
        tv.tv_usec = (ms_wait % 1000) * 1000;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        res = select(fd+1, &rfds, NULL, NULL, &tv);
        if (res == 1) {
            /* select() indicates data is available */
            res = read(fd, p, remains);

            if (res < 0 && errno != EAGAIN
#if EAGAIN != EWOULDBLOCK
                && errno != EWOULDBLOCK
#endif
                ) {
                SERIO_ERROR("serio_read: read failed");
            }

            remains -= res;
            if (remains == 0) {
                /* Read requested amount successfully */
                return l;
            }
            p += res;
        } else if (res == 0) {
            /* select() timeout */
            ms_wait = timespec_ms_remains(&duetime);
            /* Check if due time has passed */
            if (ms_wait <= 0) {
#ifdef SERIO_STRICT_READS
                SERIO_ERROR("serio_read: short read");
#else /* !SERIO_STRICT READS */
                return l - remains;
#endif
            }
#ifdef SERIO_ABORT_POLL
            /* Poll for abort */
            else if (abortpollf != NULL && abortpollf()) {
                return SERIO_ABORT;
            }
#endif
        } else {
            SERIO_ERROR("serio_read: select failed");
        }
    } /* while */
}

/*** PORT CONTROL ***/

SERIO_RETURN serio_purge(unsigned int what) {
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
}


SERIO_RETURN serio_flush(void) {
    if (tcdrain(fd) != 0) {
        SERIO_ERROR("serio_flush: tcdrain failed");
    }
    SERIO_SUCCESS();
}

/*** SETUP AND INITIALIZATION ***/

SERIO_RETURN serio_setreadtmout(unsigned int tmout) {
    readtmout = tmout;
    SERIO_SUCCESS();
}

SERIO_RETURN serio_connect(const char *fname, unsigned int baud) {
    struct termios temptermios;
    speed_t speed;

    /* OS X man page says speed corresponds to number (eg. B600 for 600 baud)
     * Linux man page doesn't say that, so maybe it doesn't, and this is
     * needed.
     */
    static const struct speedmap_s {
        unsigned int n;
        speed_t s;
    } speedmap[] = {
        /* Note B0 exists and is only used for terminating connection.*/
        { 50, B50 },
        { 75, B75 },
        { 110, B110 },
        { 134, B134 },
        { 150, B150 },
        { 200, B200 },
        { 300, B300 },
        { 600, B600 },
        { 1200, B1200 },
        { 1800, B1800 },
        { 2400, B2400 },
        { 4800, B4800 },
        { 9600, B9600 },
        { 19200, B19200 },
        { 38400, B38400 },
        { 57600, B57600 },
        { 115200, B115200 },
        { 230400, B230400 },
        { 0, 0 }
    };
    const struct speedmap_s *speedmap_p = speedmap;

    /* Find speed */
    while(1) {
        if (speedmap_p->n == 0) {
            SERIO_ERROR("serio_connect: unknown baud rate");
        } else if (speedmap_p->n == baud) {
            speed = speedmap_p->s;
            break;
        }
        speedmap_p++;
    }

    /* Open file */
    fd = open(fname, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        SERIO_ERROR("serio_open: open failed");
    }

    /* Get current tty settings */
    if (tcgetattr(fd, &temptermios) != 0) {
        close(fd);
        SERIO_ERROR("serio_open: tcgetattr failed");
    }

    /* Set parameters */
    temptermios.c_iflag = IGNBRK;
    temptermios.c_oflag = 0;
    temptermios.c_cflag = CREAD | CLOCAL | CS8 | HUPCL;
    temptermios.c_lflag = 0;
    cfsetispeed(&temptermios, speed);
    cfsetospeed(&temptermios, speed);
    if (tcsetattr(fd, TCSAFLUSH, &temptermios) != 0) {
        close(fd);
        SERIO_ERROR("serio_open: tcsetattr failed");
    }

    SERIO_SUCCESS();
}

void serio_disconnect(void) {
    close(fd);
}
