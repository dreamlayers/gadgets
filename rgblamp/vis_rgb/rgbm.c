/* Shared visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

/* Define to output averaged per-bin sums for calibration */
/* #define RGBM_LOGGING */

#include <math.h>
#include "rgbm.h"
#ifdef RGBM_LOGGING
#include <stdio.h>
#endif
#ifdef RGBM_FFTW
#include <fftw3.h>
#endif
#include "librgblamp.h"

#ifdef WIN32
#define RGBPORT "COM8"
#else
#define RGBPORT "/dev/ttyUSB0"
#endif

#if defined(RGBM_AUDACIOUS) || defined(RGBM_FFTW)

/* Calibrated in Audacious 3.4 in Ubuntu 13.10 */
/* Frequency of bin is (i+1)*44100/512 (array starts with i=0).
 * Value corresponds to amplitude (not power or dB).
 */

#if defined(RGBM_AUDACIOUS)
/* 1797/60 = 30 fps */
/* Moving average size when value is increasing */
#define RGBM_AVGUP 5
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 15
#else
/* 1000/8 = 125 fps */
/* Moving average size when value is increasing */
#define RGBM_AVGUP 21
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 63
#endif

/* Colour scaling to balance red and blue with green */
#define RGBM_REDSCALE 2.40719835270744
#define RGBM_BLUESCALE 1.60948950058501

/* Overall scaling to adapt output to values needed by librgblamp */
#ifdef RGBM_AUDACIOUS
#define RGBM_SCALE (12000.0)
#else
#define RGBM_SCALE (23500.0)
#ifdef WIN32
/* Stereo Mix stupidly scales down based on playback volume.
   AGC is needed to avoid dimness. */
#define RGBM_AGCUP (500.0)
#endif
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

#ifdef RGBM_FFTW
static fftw_plan fft_plan;
static double *fft_in, *fft_out;
static double hamming[RGBM_NUMSAMP];
#endif

/*
 * Static global variables
 */

static double binavg[3];
#ifdef RGBM_AGCUP
double pwm[3];
#endif
/* Set after successful PWM write, and enables rgb_matchpwm afterwards */
static int wrotepwm;

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

static void rgbm_avgsums(const double sums[3],
                         double avg[3],
                         double scale,
                         double bound) {
    int i = 0;
    for (i = 0; i < 3; i++) {
        double avgsize, sum;

        sum = (double)sums[i] * scale;

        if (sum > avg[i]) {
            avgsize = RGBM_AVGUP;
        } else {
            avgsize = RGBM_AVGDN;
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
    if (!rgb_open(RGBPORT))
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
    wrotepwm = 0;
    return true;
}

void rgbm_shutdown(void) {
    if (wrotepwm) {
#ifdef RGBM_AGCUP
        rgb_matchpwm_srgb(pwm[0], pwm[1], pwm[2]);
#else
        rgb_matchpwm(binavg[0], binavg[1], binavg[2]);
#endif
    }
    rgb_close();
#ifdef RGBM_FFTW
    fftw_destroy_plan(fft_plan);
    fftw_free(fft_in);
    fftw_free(fft_out);
#endif
#ifdef RGBM_LOGGING
    if (testlog != NULL) fclose(testlog);
#endif
}

#ifdef RGBM_FFTW
static
#endif
int rgbm_render(const RGBM_BINTYPE bins[RGBM_NUMBINS]) {
    double sums[3];
    int res;

    rgbm_sumbins(bins, sums);
    sums[0] *= RGBM_REDSCALE;
    sums[2] *= RGBM_BLUESCALE;
    rgbm_avgsums(sums, binavg, RGBM_SCALE, RGBM_LIMIT);
    rgb_flush();
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
    res = rgb_pwm_srgb(pwm[0], pwm[1], pwm[2]);
#else
    res = rgb_pwm(binavg[0], binavg[1], binavg[2]);
#endif
    if (res) wrotepwm = 1;
    return res;
} /* rgbm_render */

#ifdef RGBM_FFTW
/* Convert FFTW halfcomplex format to real amplitudes.
 * bins[0] and bins[RGBM_NUMSAMP / 2] are real due to FFT symmetry. */
static void fft_complex_to_real(RGBM_BINTYPE bins[RGBM_NUMSAMP]) {
    int i;
    for (i = 1; i < RGBM_NUMSAMP / 2; i++) {
        double t;
        t = bins[i] * bins[i];
        t += bins[RGBM_NUMSAMP - i] * bins[RGBM_NUMSAMP - i];
        bins[i - 1] = sqrt(t) / RGBM_NUMSAMP;
    }
    bins[RGBM_NUMSAMP / 2 - 1] = fabs(bins[RGBM_NUMSAMP / 2]) / RGBM_NUMSAMP;
}

RGBM_BINTYPE *rgbm_get_wave_buffer(void) {
    return fft_in;
}

static void fft_apply_window(double *samp) {
    int i;
    for (i = 0; i < RGBM_NUMSAMP; i++) {
      samp[i] *= hamming[i];
    }
}

int rgbm_render_wave(void) {
    fft_apply_window(fft_in);
    fftw_execute(fft_plan);
    fft_complex_to_real(fft_out);
    return rgbm_render(fft_out);
}
#endif
