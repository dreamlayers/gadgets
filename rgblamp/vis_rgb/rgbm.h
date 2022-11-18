/* Header file for RGB lamp visualization plugin. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#ifndef _RGBM_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(RGBM_FFTW)

/* Number of FFT bins */
#define RGBM_NUMBINS 256
/* Type of FFT bins */
#define RGBM_BINTYPE double
/* Number of samples used as input to FFT to produce bins */
#define RGBM_NUMSAMP (RGBM_NUMBINS*2)

#elif defined(RGBM_AUDACIOUS)

#ifdef WIN32
/* Stereo Mix stupidly scales down based on playback volume.
   AGC is needed to avoid dimness. */
#define RGBM_AGCUP (500.0)
#endif

#define RGBM_NUMBINS 256
#define RGBM_BINTYPE float

#elif defined (RGBM_WINAMP)

#define RGBM_NUMBINS 576
#define RGBM_BINTYPE unsigned char

#else
#error Need to set define for type of music player.
#endif

/* Here int really means bool, but some compilers can't handle bool */
int rgbm_init(void);
void rgbm_shutdown(void);
#if !defined(RGBM_FFTW)
int rgbm_render(const RGBM_BINTYPE bins[RGBM_NUMBINS]);
#else
RGBM_BINTYPE *rgbm_get_wave_buffer(void);
int rgbm_render_wave(double deltat);
#endif

/* Standalone functions */
int rgbm_pollquit(void);
void rgbm_run(const char *snddev);

/* Hardware driver functions */
int rgbm_hw_open(void);
#ifdef RGBM_AGCUP
int rgbm_hw_srgb(const double *rgb);
#else
int rgbm_hw_pwm(const double *rgb);
#endif
void rgbm_hw_close(void);

#ifdef __cplusplus
}
#endif

#endif /* !_RGBM_H_ */
