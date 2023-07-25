/* Shared visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

/* Define to output averaged per-bin sums for calibration */
/* #define RGBM_LOGGING */

#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "rgbm.h"
#ifdef RGBM_LOGGING
#include <stdio.h>
#endif
#ifdef RGBM_FFTW
#include <fftw3.h>
#endif
#include <stdbool.h>

/*
 * Configuration you can tweak
 */

/* These exponential moving average coefficients are for 1 FPS.
 * Divide by time in seconds since last frame to get coefficient for
 * current frame.
 */
#ifndef RGBM_WINAMP
/* Moving average size when value is increasing */
#define RGBM_AVGUP 0.16695
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 0.50085
#endif

#if defined(RGBM_AUDACIOUS) || defined(RGBM_FFTW)

/* Calibrated in Audacious 3.4 in Ubuntu 13.10 */
/* Frequency of bin is (i+1)*44100/512 (array starts with i=0).
 * Value corresponds to amplitude (not power or dB).
 */

/* Audacious 3.7.2 gave 0.03339 seconds per frame (about 30 fps), and
 * exponential moving average size was 5 down and 15 up.
 * Standalone in Linux gave 0.00774 seconds per frame, and
 * exponential moving average size was 21 down and 63 up.
 */

/* Colour scaling to balance red and blue with green */
#define RGBM_REDSCALE 2.40719835270744
#define RGBM_BLUESCALE 1.60948950058501

/* Overall scaling to adapt output to values needed by librgblamp */
#ifdef RGBM_AUDACIOUS
#define RGBM_SCALE (12000.0)
#else
#define RGBM_SCALE (23500.0)
#endif
#define RGBM_LIMIT 4095

/* Table for bin weights for summing bin powers (amplitued squared) to green.
 * Below RGBM_PIVOTBIN, red + green = 1.0. At and above, red + blue = 1.0.
 */
#include "greentab_audacious.h"

/* Table for adjusting bin amplitudes using equal loudness contour. */
#define HAVE_FREQ_ADJ
static const double freq_adj[RGBM_USEBINS] = {
#include "freqadj_audacious.h"
};

#elif defined(RGBM_WINAMP)

/* Calibrated in Winamp v5.666 Build 3512 (x86)
 * Frequency of bin roughly corresponds to (i - 1) * 44100 / 1024
 * This doesn't behave like a good FFT. Increased amplitude causes
 * peak to broaden more than it grows. I don't know the mathematical
 * connection between the signal and bins.
 */

/* Moving average size when value is increasing */
#define RGBM_AVGUP 7
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 20

/* Colour scaling to balance red and blue with green.
 * Calibrated using pink noise, after setting above parameters.
 */
#define RGBM_REDSCALE 2.25818534475793
#define RGBM_BLUESCALE 1.337960735802697

/* Overall scaling to adapt output to values needed by librgblamp */
#define RGBM_SCALE (3.5)
#define RGBM_LIMIT 4095

#include "greentab_winamp.h"

#else
#error Need to set define for type of music player.
#endif

/*
 * Automatic configuration
 */

#ifdef RGBM_FFTW
#define RGBM_STANDALONE
#endif

#ifdef RGBM_STANDALONE
#define IF_STANDALONE(x...) x
#else
#define IF_STANDALONE(x...)
#endif

/*
 * Static global variables
 */

#ifdef RGBM_FFTW
static fftw_plan fft_plan;
static double *fft_in, *fft_out;
static double hamming[RGBM_NUMSAMP];
#endif

static double binavg[3];
#ifdef RGBM_AGCUP
static double pwm[3];
#endif

#ifdef RGBM_LOGGING
static double testsum[RGBM_USEBINS];
static unsigned int testctr;
FILE *testlog = NULL;
static double avgavg[3];
#define RGBM_TESTAVGSIZE 1000
#endif

/*
 * Internal routines
 */

/* Sum bins into R, G, B colours. Bins before RGBM_PIVOTBIN sum into R and G.
 * Bins starting at RGBM_PIVOTBIN and before RGBM_USEBINS sum into G and B.
 * The green_tab array determines what portion of bin goes into G, and the
 * rest goes into the other colour.
 */
static void rgbm_sumbins(const RGBM_BINTYPE bins[RGBM_NUMBINS],
                         double sums[3]){
    double greensum = 0;
    int rgb, i = 0, limit = RGBM_PIVOTBIN;
    /* First, sum other to red before pivot.
     * Then sum other to blue from pivot to end
     */
    for (rgb = 0; rgb < 3; rgb += 2) {
        double othersum = 0;
        for (; i < limit; i++) {
            double green, bin = bins[i];
#ifdef HAVE_FREQ_ADJ
            bin *= freq_adj[i];
#endif
            bin *= bin;
            green = green_tab[i] * bin;
            greensum += green;
            othersum += bin - green;
        }
        sums[rgb] = sqrt(othersum);
        limit = RGBM_USEBINS;
    }
    sums[1] = sqrt(greensum);
} /* rgbm_sumbins */

#if !defined(RGBM_STANDALONE) && !defined(RGBM_WINAMP)
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

static double calc_deltat(void) {
    static struct timeval lasttime;
    static int haveltime = 0;
    struct timeval thistime;
    double deltat;

    /* Calculate length of time since last frame */
    gettimeofday(&thistime, NULL);
    if (haveltime) {
        struct timeval deltatv;
        if (timeval_subtract(&deltatv, &thistime, &lasttime) == 0) {
            deltat = deltatv.tv_sec + (double)deltatv.tv_usec / 1000000.0;
        } else {
            deltat = 0.0;
        }
    } else {
        deltat = 1.0;
        haveltime = 1;
    }
    lasttime.tv_sec = thistime.tv_sec;
    lasttime.tv_usec = thistime.tv_usec;

    return deltat;
}
#endif

/* Scale R, G, B sums from rgbm_sumbins(), and perform an exponential moving
 * average. Moving average coefficient changes between RGBM_AVGUP and
 * RGBM_AVGDN depending on whether current sum is higher or lower than average.
 * Clipping to bound is performed if AGC is not in use.
 */
static void rgbm_avgsums(const double sums[3],
                         double avg[3],
                         double scale,
                         double bound
                         IF_STANDALONE(, double deltat)
                         ) {
    int i = 0;
    double avgup, avgdn;
#if !defined(RGBM_STANDALONE) && !defined(RGBM_WINAMP)
    double deltat = calc_deltat();
#endif

    /* Calculate exponential moving average coefficients
     * based on time since last frame.
     */
#ifdef RGBM_WINAMP
    avgup = RGBM_AVGUP;
    avgdn = RGBM_AVGDN;
#else
    if (deltat == 0.0) return;
    avgup = RGBM_AVGUP / deltat;
    if (avgup <= 1.0) avgup = 1.0;
    avgdn = RGBM_AVGDN / deltat;
    if (avgdn <= 1.0) avgdn = 1.0;
#endif

    /* Scale sums and add them to exponential moving average */
    for (i = 0; i < 3; i++) {
        double avgsize, sum;

        sum = (double)sums[i] * scale;

        if (sum > avg[i]) {
            avgsize = avgup;
        } else {
            avgsize = avgdn;
        }

        avg[i] = ((avgsize - 1.0) * avg[i] + sum) / avgsize;
#ifndef RGBM_AGCUP
        /* Clip to bound without AGC. Otherwise AGC avoids clipping. */
        if (avg[i] > bound) avg[i] = bound;
#endif
    } /* for i */
} /* rgbm_avgsums */

#ifdef RGBM_AGCUP
static void rgbm_agc(const double avg[3], double pwm[3]) {
    static double scale = 1.0 / 4095;
    /* Limit scale during low volume and avoids division by zero */
    double maxval = 1.0;
    int i;

    /* Calculate scale based on maximum of R, G, B values */
    for (i = 0; i < 3; i++) {
        if (avg[i] > maxval) maxval = avg[i];
    }

    maxval = 1.0 / maxval;
    if (maxval < scale) {
        /* Peaks reset scale immediately */
        scale = maxval;
    } else {
        /* Otherwise scale can go up slowly according to RGBM_AGCUP */
        scale = (RGBM_AGCUP - 1) / RGBM_AGCUP * scale +
                1 / RGBM_AGCUP * maxval;
    }

    /* Scale PWM values based on scale */
    for (i = 0; i < 3; i++) {
        pwm[i] = avg[i] * scale;
    }
}
#endif /* RGBM_AGCUP */

#ifdef RGBM_LOGGING
static void rgbm_testsum(const RGBM_BINTYPE bins[RGBM_NUMBINS]) {
    int i;

    if (testlog == NULL) return;

    for (i = 0; i < RGBM_USEBINS; i++) {
        double bin = bins[i];
#ifdef HAVE_FREQ_ADJ
        bin *= freq_adj[i];
#endif
        testsum[i] = (testsum[i] * (RGBM_TESTAVGSIZE - 1) + bin)
                     / RGBM_TESTAVGSIZE;
    }

    if (testctr++ >= RGBM_TESTAVGSIZE) {
        testctr = 0;
        for (i = 0; i < RGBM_USEBINS; i++) {
            fprintf(testlog, "%i:%f\n", i, testsum[i]);
        }
    }
} /* RGBM_LOGGING */
#endif /* RGBM_LOGGING */

/*
 * Interface routines
 */

int rgbm_init(void) {
    int i;
    if (!rgbm_hw_open())
        return false;

#ifdef RGBM_FFTW
    fft_in = (double *)fftw_malloc(sizeof(double) * RGBM_NUMSAMP);
    fft_out = (double *)fftw_malloc(sizeof(double) * RGBM_NUMSAMP);
    fft_plan = fftw_plan_r2r_1d(RGBM_NUMSAMP,
                                fft_in, fft_out,
                                FFTW_R2HC,
                                FFTW_ESTIMATE | FFTW_DESTROY_INPUT);
    for (i = 0; i < RGBM_NUMSAMP; i++) {
        /* This has been scaled to maintain amplitude */
        hamming[i] = 1 - 0.852 * cos(2 * M_PI * i / (RGBM_NUMSAMP - 1));
    }
#endif

    for (i = 0; i < 3; i++) binavg[i] = 0.0;
#ifdef RGBM_LOGGING
    for (i = 0; i < RGBM_USEBINS; i++) testsum[i] = 0.0;
    testlog = fopen("rgbm.log", "w");
    testctr = 0;
#endif
    return true;
}

void rgbm_shutdown(void) {
    rgbm_hw_close();
#ifdef RGBM_FFTW
    fftw_destroy_plan(fft_plan);
    fftw_free(fft_in);
    fftw_free(fft_out);
#endif
#ifdef RGBM_LOGGING
    if (testlog != NULL) fclose(testlog);
#endif
}

/* The function tying it all together, taking new FFT bins as input,
 * calling calculation functions, and setting new LED colours.
 */
#ifdef RGBM_FFTW
static
#endif
int rgbm_render(const RGBM_BINTYPE bins[RGBM_NUMBINS]
                IF_STANDALONE(, double deltat))
{
    double sums[3];
    int res;

    rgbm_sumbins(bins, sums);
    sums[0] *= RGBM_REDSCALE;
    sums[2] *= RGBM_BLUESCALE;
    rgbm_avgsums(sums, binavg, RGBM_SCALE, RGBM_LIMIT IF_STANDALONE(, deltat));

#ifdef RGBM_LOGGING
    for (res = 0; res < 3; res++) {
        avgavg[res] = (avgavg[res] * (RGBM_TESTAVGSIZE-1) + binavg[res])
                      / RGBM_TESTAVGSIZE;
    }
    if (testlog != NULL) {
        if (testctr == RGBM_TESTAVGSIZE-1) {
            fprintf(testlog, "%9f; %9f; %9f\n", avgavg[0], avgavg[1], avgavg[2]);
        }
        rgbm_testsum(bins);
        fprintf(testlog, "%9f, %9f, %9f\n", binavg[0], binavg[1], binavg[2]);
    }
#endif
#ifdef RGBM_AGCUP
    rgbm_agc(binavg, pwm);
    /* AGC washes out the colours if the output is used as raw PWM,
       so sRGB coversion is used for gamma correction. */
    res = rgbm_hw_srgb(pwm);
#else
    res = rgbm_hw_pwm(binavg);
#endif
    return res;
} /* rgbm_render */

#ifdef RGBM_FFTW
/* Convert FFTW halfcomplex format to real amplitudes.
 * bins[0] and bins[RGBM_NUMSAMP / 2] are real due to FFT symmetry. */
static void fft_complex_to_real(RGBM_BINTYPE bins[RGBM_NUMSAMP],
                                RGBM_BINTYPE res[RGBM_NUMSAMP / 2]) {
    int i;
    for (i = 1; i < RGBM_NUMSAMP / 2; i++) {
        double t;
        t = bins[i] * bins[i];
        t += bins[RGBM_NUMSAMP - i] * bins[RGBM_NUMSAMP - i];
        res[i - 1] = sqrt(t) / RGBM_NUMSAMP;
    }
    res[RGBM_NUMSAMP / 2 - 1] = fabs(bins[RGBM_NUMSAMP / 2]) / RGBM_NUMSAMP;
}

/* It may seem more logical to pass data to rgbm_render_wave() using a
 * parameter, but this buffer is allocated via fftw_malloc(), and this
 * avoids memcpy().
 */
RGBM_BINTYPE *rgbm_get_wave_buffer(void) {
    return fft_in;
}

static void fft_apply_window(double *samp) {
    int i;
    for (i = 0; i < RGBM_NUMSAMP; i++) {
      samp[i] *= hamming[i];
    }
}

/* Alternate entry point when input is wave data, and not FFT bins. The wave
 * data first needs to be copied to buffer returned by rgbm_get_wave_buffer(),
 * and then this function is called.
 */
int rgbm_render_wave(double deltat) {
    static RGBM_BINTYPE res[3][RGBM_NUMSAMP / 2];
    static int this = 0, last = 1;
    int i;

    fft_apply_window(fft_in);
    fftw_execute(fft_plan);
    fft_complex_to_real(fft_out, res[this]);
    for (i = 0; i < RGBM_NUMSAMP / 2; i++) {
        res[2][i] = res[this][i] * 2 - res[last][i];
    }
    if (this) {
        this = 0;
        last = 1;
    } else {
        this = 1;
        last = 0;
    }
    return rgbm_render(res[2], deltat);
}
#endif
