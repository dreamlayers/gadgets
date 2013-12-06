#ifndef _RGBM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Define to output averaged per-bin sums for calibration */
/* #define RGBM_LOGGING */

#if defined(RGBM_AUDACIOUS)

#define RGBPORT "/dev/ttyUSB0"

/* Number of FFT bins */
#define RGBM_NUMBINS 256
/* Type of FFT bins */
#define RGBM_BINTYPE float

/* Moving average size when value is increasing */
#define RGBM_AVGUP 5
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 15

#define RGBM_SCALE (3000.0 / 4095.0)

#elif defined (RGBM_WINAMP)

#define RGBPORT "COM8"

/* Number of FFT bins */
#define RGBM_NUMBINS 576
/* Type of FFT bins */
#define RGBM_BINTYPE unsigned char

/* Moving average size when value is increasing */
#define RGBM_AVGUP 7
/* Moving average size when value is decreasing */
#define RGBM_AVGDN 20

#define RGBM_SCALE (1.0 / 4095.0)

#else
#error Need to set define for type of music player.
#endif

/* Here int really means bool, but some compilers can't handle bool */
int rgbm_init(void);
void rgbm_shutdown(void);
int rgbm_render(const RGBM_BINTYPE bins[RGBM_NUMBINS]);

#ifdef __cplusplus
}
#endif

#endif /* !_RGBM_H_ */
