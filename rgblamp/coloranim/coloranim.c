#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "coloranim.h"

static pixel temp_pix = NULL, cur_pix = NULL;

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

void *safe_malloc(unsigned int l)
{
    void *p = malloc(l);
    if (p == NULL) fatal("out of memory");
    return p;
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
    pixel res = safe_malloc(sizeof(double) * COLORCNT * PIXCNT);
    return res;
}

void pix_clear(pixel pix) {
    int i;
    for (i = 0; i < COLORCNT * PIXCNT; i++) {
        pix[i] = 0.0;
    }
}

void pix_free(pixel *p) {
    if (*p != NULL) {
        free(*p);
        *p = NULL;
    }
}

static void pix_copy(pixel dst, pixel src) {
    memcpy(dst, src, sizeof(double) * COLORCNT * PIXCNT);
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
static int interp_gradient(const pixel a, const pixel b, pixel r)
{
    unsigned int i, j;
    for (i = 0; i < COLORCNT; i++) {
        if (a[i] < 0 || b[i] < 0) {
            return -1;
        }
    }
    for (i = 0; i < (PIXCNT * 3); i += 3) {
        for (j = 0; j < COLORCNT; j++) {
            r[i + j] = interp_one(a[j], b[j],
                       (double)i / (PIXCNT * 3 - 3));
        }
    }
    return 0;
}
#endif

static void fx_fill(const pixel last_pix, const pixel clr,
                    pixel dest, int count)
{
    unsigned int i, j, end = count * 3;
    for (i = 0; i < end; i += 3) {
        for (j = 0; j < COLORCNT; j++) {
            dest[i + j] = (clr[j] >= 0.0) ? clr[j] : last_pix[i + j];
        }
    }
}

int fx_makestate(const pixel colorspec, const keyword *colorkw,
                 unsigned int numspec,
                 const pixel last_pix, pixel dest)
{
    switch (colorkw[0]) {
    case KW_NONE:
        if (numspec == 1) {
            fx_fill(&last_pix[0], &colorspec[0], dest, PIXCNT);
        } else {
            return -1; /* color keyword expected */
        }
        break;
#if PIXCNT > 1
    case KW_GRADIENT:
        if (numspec == 2) {
            return interp_gradient(&colorspec[0], &colorspec[COLORCNT], dest);
        } else {
            return -1; /* multiple point gradient unimplemented */
        }
        break;
    case KW_SOLID:
        if (numspec == 2) {
            fx_fill(&last_pix[0], &colorspec[0], &dest[0], PIXCNT / 2);
            fx_fill(&last_pix[COLORCNT], &colorspec[COLORCNT],
                    &dest[(PIXCNT / 2) * COLORCNT], PIXCNT - PIXCNT / 2);
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

static int fx_crossfade(const pixel oldclr, const pixel newclr,
                        double seconds)
{
    stopwatch_start();
    while (1) {
        double t;

        t = stopwatch_elapsed();
        if (t >= seconds) {
            render(newclr);
            return 1;
        } else {
            interp_fade(oldclr, newclr, t / seconds, temp_pix);
            /* pix_print(cross); */
            render(temp_pix);
        }

        if (cmd_cb_pollquit()) {
            return 0;
        }
    }
}

static int fx_wait(double seconds)
{
    stopwatch_start();
    while (1) {
        double t;

        t = seconds - stopwatch_elapsed();
        if (t <= 0) {
            /* Wait completed */
            return 1;
        } else {
#define MAX_SLEEP 10000
            if (t > MAX_SLEEP / 1000000) {
                usleep(MAX_SLEEP);
            } else {
                usleep(t * 1000000);
            }
        }

        if (cmd_cb_pollquit()) {
            /* Wait interrupted */
            return 0;
        }
    }
}

void coloranim_init(void)
{
    temp_pix = pix_alloc();
    cur_pix = pix_alloc();
    render_get(cur_pix);
}

void coloranim_quit(void)
{
    pix_free(&temp_pix);
    pix_free(&cur_pix);
}

void coloranim_exec(struct coloranim *ca)
{
    pixel last_pix = cur_pix;
    struct coloranim_state *state = ca->states;
    double fade = ca->first_fade;
    pixel save_pix = NULL;

    if (state == NULL) fatal("no states in animation");

    while(!cmd_cb_pollquit()) {
        if (fade > 0.0) {
            /* TODO: Fading between solid colours could be optimized */
            if (fx_crossfade(last_pix, state->pix, fade) == 0) {
                /* Fade interrupted */
                save_pix = temp_pix;
                break;
            } else {
                /* Fade completed */
                save_pix = state->pix;
            }
        } else if (fade == 0.0) {
            render(state->pix);
            save_pix = state->pix;
        } else {
            fatal("negative fade duration");
        }

        if (state->hold_for > 0) {
            if (fx_wait(state->hold_for) == 0) break;
        }

        last_pix = state->pix;
        state = state->next;
        if (state == NULL) {
            if (ca->repeat) {
                state = ca->states;
            } else {
                break;
            }
        }
        fade = state->fade_to;
    }
    if (save_pix != NULL) {
        pix_copy(cur_pix, save_pix);
    }
}

int parse_and_run(void) {
    struct coloranim *ca;

    ca = coloranim_parse();
    if (ca == NULL) return -1;
    coloranim_exec(ca);
    coloranim_free(&ca);
    return 0;
}
