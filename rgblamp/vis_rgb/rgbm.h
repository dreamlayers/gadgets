#ifndef _RGBM_H_

/* Define to output averaged per-bin sums for calibration */
/* #define RGBM_TESTSUM */

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

#else
#error Need to set define for type of music player.
#endif

/* Here int really means bool, but some compilers can't handle bool */
int rgbm_init(void);
void rgbm_shutdown(void);
int rgbm_render(const RGBM_BINTYPE bins[RGBM_NUMBINS]);

#endif /* !_RGBM_H_ */
