#ifndef _RGBCLIENT_H_
#define _RGBCLIENT_H_

#if defined(WIN32) || defined(__CYGWIN__)
#ifdef BUILDING_LIBVFD
#ifdef BUILDING_DLL
#define RGBAPI __declspec(dllexport)
#else
#define RGBAPI
#endif
#else
/* This breaks static linking.
 * #define LEDAPI __declspec(dllimport)
 */
#define RGBAPI
#endif
#else /* !defined(WIN32) && !defined(__CYGWIN__) */
#define RGBAPI
#endif

#ifdef __cplusplus
extern "C" {
#else
#ifdef _MSC_VER
#include <windows.h>
#define bool BOOL
#define true TRUE
#define false FALSE
#else
#include <stdbool.h>
#endif
#endif

/*** Commands ***/

/* These are typically needed because they are encapsulated
 * by functions below
 */
#define SERCMD_R_RGBOUT 0
#define SERCMD_W_RGBOUT 1
#define SERCMD_R_RGBVAL 2
#define SERCMD_W_RGBVAL 3
#define SERCMD_R_POTS 4
#define SERCMD_W_DOT 5
#define SERCMD_R_DOT 6

/* Switch bits for right switch in PWM values */

#define RGB_RSW_MID 8
#define RGB_RSW_UP 2

/*** Writing functions ***/

/* Send raw PWM values (0-4095) */
RGBAPI bool rgb_pwm(unsigned short r, unsigned short g, unsigned short b);

/* Start a thread which sends PWM values, so rgb_pwm returns immediately */
RGBAPI bool rgb_beginasyncpwm(void);

/* Stop PWM value sending thread, so rgb_pwm returns immediately */
RGBAPI void rgb_endasyncpwm(void);

/* Send 16 bit 0 to 0xFFFF values corresponding to 0 to 1.0.
 * They are squared before being converted to PWM values.
 */
RGBAPI bool rgb_squared(unsigned short r, unsigned short g, unsigned short b);

/* Set dot correction values (0-63) */
RGBAPI bool rgb_dot(unsigned char r, unsigned char g, unsigned char b);

/* Send 0 to 1.0 floating point sRGB values, which are converted to
 * raw PWM values before being sent.
 */
RGBAPI bool rgb_pwm_srgb(double r, double g, double b);

/* Send 0 to 255 sRGB values, which are converted to
 * raw PWM values before being sent.
 */
RGBAPI bool rgb_pwm_srgb256(unsigned char r, unsigned char g, unsigned char b);

/* Set raw PWM values via rgb_squared, so fading works after */
RGBAPI bool rgb_matchpwm(unsigned short r, unsigned short g, unsigned short b);

/* Same as rgb_matchpwm, taking 0 to 1.0 sRGB input */
RGBAPI bool rgb_matchpwm_srgb(double r, double g, double b);

/* Same as rgb_matchpwm, taking 0 to 255 sRGB input */
RGBAPI bool rgb_matchpwm_srgb256(unsigned char r,
                                 unsigned char g,
                                 unsigned char b);

/* Use rgbout to re-set PWM values via rgb_squared, so fading works after */
RGBAPI bool rgb_fadeprep(void);

/*** Reading functions ***/

/* Get raw values that were output to TLC5940
 * These are raw PWM values unless the last function was dot correction.
 */
RGBAPI bool rgb_getout(unsigned short *dest);

/* Same as rgb_getout, but returning 0.0 to 1.0 sRGB value */
RGBAPI bool rgb_getout_srgb(double *dest);

/* Same as rgb_getout, but returning 0 to 255 sRGB value */
RGBAPI bool rgb_getout_srgb256(unsigned char *dest);

/* Send command, get 3 values. Use defines below */
RGBAPI bool rgb_getnormal(unsigned char cmd, unsigned short *dest);

/* Get not gamma corrected values (3 values, 0-0xFFFF) */
#define rgb_getval(dest) rgb_getnormal(SERCMD_R_RGBVAL,dest)

/* Get pots and switches */
#define rgb_getpots(dest) rgb_getnormal(SERCMD_R_POTS,dest)

/* Get dot correction values */
RGBAPI bool rgb_getdot(unsigned short *dest);

/*** Initialization functions ***/

RGBAPI void rgb_close(void);

RGBAPI bool rgb_open(const char *fn);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
