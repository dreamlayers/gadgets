#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include "coloranim.h"

#ifdef WIN32
/* FIXME Unused SOCKS part of libmosquitto needs this. */
void inet_pton(void) { exit(-1); }
#endif

void fatal(const char *s) __attribute__ ((noreturn));
void fatal(const char *s)
{
    fprintf(stderr, "Fatal error: %s\n", s);
    exit(-1);
}

/* --- Timing section --- */

/* From https://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html */
static int
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

static struct timeval starttime;
static void stopwatch_start(void) {
    gettimeofday(&starttime, NULL);
}

static double stopwatch_elapsed(void) {
    struct timeval thistime;

    gettimeofday(&thistime, NULL);
    struct timeval deltatv;
    if (timeval_subtract(&deltatv, &thistime, &starttime) == 0) {
        return deltatv.tv_sec + (double)deltatv.tv_usec / 1000000.0;
    } else {
        return 0.0;
    }
}

pixel pix_alloc(void) {
    pixel res = malloc(sizeof(double) * COLORCNT * PIXCNT);
    if (res == NULL) fatal ("allocating pixel data");
    return res;
}

void pix_free(pixel *p) {
    if (*p != NULL) {
        free(*p);
        *p = NULL;
    }
}

#ifdef DEBUG
static void pix_print(pixel pix) {
    unsigned int i, j;
    for (i = 0; i < PIXCNT; i++) {
        DEBUG_PRINT("%u: ", i);
        for (j = 0; j < COLORCNT; j++) {
            DEBUG_PRINT("%f, ", pix[i * COLORCNT + j]);
        }
        DEBUG_PRINT("\n");
    }
}
#endif /* DEBUG */

/* p = idx / (len - 1) */
static inline double interp_one(double a, double b, double p)
{
    return a * (1.0 - p) + b * p;
}

static void interp_fade(const pixel a, const pixel b, double p, pixel r)
{
    unsigned int i, cnt = COLORCNT * PIXCNT;
    for (i = 0; i < cnt; i++) {
        r[i] = interp_one(a[i], b[i], p);
    }
}

#if PIXCNT > 1
static void interp_gradient(const pixel a, const pixel b, pixel r)
{
    unsigned int i, j;
    for (i = 0; i < (PIXCNT * 3); i += 3) {
        for (j = 0; j < COLORCNT; j++) {
            r[i + j] = interp_one(a[j], b[j],
                       (double)i / (PIXCNT * 3 - 3));
        }
    }
}
#endif

static void fx_fill(const pixel clr, pixel dest)
{
    unsigned int i, j;
    for (i = 0; i < (PIXCNT * 3); i += 3) {
        for (j = 0; j < COLORCNT; j++) {
            dest[i + j] = clr[j];
        }
    }
}

int fx_makestate(const pixel colorspec, const keyword *colorkw,
                 unsigned int numspec,
                 pixel dest)
{
    switch (colorkw[0]) {
    case KW_NONE:
        if (numspec == 1) {
            fx_fill(&colorspec[0], dest);
        } else {
            return -1; /* color keyword expected */
        }
        break;
#if PIXCNT > 1
    case KW_GRADIENT:
        if (numspec == 2) {
            interp_gradient(&colorspec[0], &colorspec[COLORCNT], dest);
        } else {
            return -1; /* multiple point gradient unimplemented */
        }
        break;
#endif
    default:
        return -1; /* bad color keyword */
        break;
    }
    return 0;
}

static void fx_crossfade(const pixel oldclr, const pixel newclr, double seconds)
{
    pixel cross = pix_alloc();

    stopwatch_start();
    while (!cmd_cb_pollquit()) {
        double t = stopwatch_elapsed();
        if (t > seconds) break;
        interp_fade(oldclr, newclr, t / seconds, cross);
        /* pix_print(cross); */
        render(cross);
    }

    free(cross);
}

int fx_transition(const pixel oldclr, keyword kw, double arg,
                  const pixel newclr)
{
    switch (kw) {
    case KW_NONE:
#ifdef DEBUG
        pix_print(newclr);
#endif /* DEBUG */
        render(newclr);
        break;
    case KW_CROSSFADE:
        fx_crossfade(oldclr, newclr, arg);
        break;
    default:
        return -1; /* bad transition keyword" */
        break;
    }
    return 0;
}
