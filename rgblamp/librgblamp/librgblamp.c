// rgbclient.cpp : Defines the entry point for the console application.

#include <stdlib.h>
//#include <stdio.h>
#include <math.h>
#ifdef WIN32
#include <windows.h>
#endif
#include "serio.h"
#include <stdio.h>
#define BUILDING_LIBRGBLAMP
#include "librgblamp.h"

/*** WRITING ***/

#ifdef WIN32
static CRITICAL_SECTION rgbcs;    // Don't send part-old, part-new value
static HANDLE rgbdataevt;  // Tell SettingThread about new data
static bool doasyncpwm = false;
static HANDLE settingthreadh = NULL;
static bool asyncpwmfail = false;
#endif // WIN32
static unsigned char rgbbuf[5]; // Pack data into here

// Send PWM values to lamp
RGBAPI bool rgb_pwm(unsigned short r, unsigned short g, unsigned short b) {
    if (r > 4095) r = 4095;
    if (g > 4095) g = 4095;
    if (b > 4095) b = 4095;

#ifdef WIN32
    if (doasyncpwm) {
        EnterCriticalSection(&rgbcs);
    }
#endif

    rgbbuf[0] = ((r << 4) & 0xF0) | SERCMD_W_RGBOUT;
    rgbbuf[1] = (r >> 4) & 0xFF;
    rgbbuf[2] = ((g << 4) & 0xF0) | ((b >> 8) & 0x0F);
    rgbbuf[3] = (g >> 4) & 0xFF;
    rgbbuf[4] = b & 0xFF;

#ifdef WIN32
    if (doasyncpwm) {
        bool failed = asyncpwmfail;
        asyncpwmfail = false;
        LeaveCriticalSection(&rgbcs);
        SetEvent(rgbdataevt);
        return failed;
    } else
#endif
    {
        return serio_write(rgbbuf, 5) >= 0;
    }
}

#ifdef WIN32
// Thread which asynchronously sends PWM values to lamp
static DWORD WINAPI SettingThread(LPVOID lpParameter) {
    unsigned char buf[5];
    while (1) {
        if (WaitForSingleObject(rgbdataevt, INFINITE) != WAIT_OBJECT_0) {
            asyncpwmfail = true;
            return -1;
        }

        if (!doasyncpwm) return 0;

        EnterCriticalSection(&rgbcs);
        memcpy(buf, rgbbuf, 5);
        LeaveCriticalSection(&rgbcs);

        if (serio_write(buf, 5) < 0) {
            EnterCriticalSection(&rgbcs);
            asyncpwmfail = true;
            LeaveCriticalSection(&rgbcs);
        }
    }
}

// Begin asynchronous PWM value sending
RGBAPI bool rgb_beginasyncpwm(void) {
    if (!InitializeCriticalSectionAndSpinCount(&rgbcs, 0x80000400)) return false;
    doasyncpwm = true;
    rgbdataevt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (rgbdataevt == NULL) {
        rgb_endasyncpwm();
        return false;
    }
    settingthreadh = CreateThread(NULL, 0 , SettingThread, NULL, 0, NULL);
    if (settingthreadh == NULL) {
        rgb_endasyncpwm();
        return false;
    }
    return true;
}

// End asynchronous PWM value sending
RGBAPI void rgb_endasyncpwm(void) {
    bool delcs = doasyncpwm;

    doasyncpwm = false;        // Get SettingThread to quit
    if (settingthreadh != NULL && SetEvent(rgbdataevt)) {
        WaitForSingleObject(settingthreadh, INFINITE);    // Wait for it to quit
        CloseHandle(settingthreadh);
        settingthreadh = NULL;
    }

    if (delcs) {
        DeleteCriticalSection(&rgbcs);
    }

    if (rgbdataevt != NULL) {
        CloseHandle(rgbdataevt);
        rgbdataevt = NULL;
    }
}
#endif

// Send three 16 bit values in 7 bytes
static bool rgb_unpacked(unsigned char cmd, unsigned short r, unsigned short g, unsigned short b) {
    unsigned char buf[7];
    buf[0] = cmd;
    buf[1] = r & 0xFF;
    buf[2] = (r >> 8) & 0xFF;
    buf[3] = g & 0xFF;
    buf[4] = (g >> 8) & 0xFF;
    buf[5] = b & 0xFF;
    buf[6] = (b >> 8) & 0xFF;
    return serio_write(buf, 7) >= 0;
}

// Send values which are squared before being used as PWM values
RGBAPI bool rgb_squared(unsigned short r, unsigned short g, unsigned short b) {
    return rgb_unpacked(SERCMD_W_RGBVAL, r, g, b);
}

// Send dot correction
RGBAPI bool rgb_dot(unsigned char r, unsigned char g, unsigned char b) {
    return rgb_unpacked(SERCMD_W_DOT, r << 10, g << 10, b << 10);
}

/*** sRGB ***/

#ifdef _MSC_VER
static __inline long int lrint (double flt) {
    int intgr;
    _asm {
        fld flt
        fistp intgr
    };
    return intgr;
}

#if 0
static __inline long int lrintf (float flt) {
    int intgr;
    _asm {
        fld flt
        fistp intgr
    };
  return intgr;
}
#endif
#endif

static unsigned short rgb_srgb2pwm(double n) {
    double r;
    if (n <= 0.04045) {
        r = n / 12.92;
    } else {
        r = pow((n+0.055)/1.055, 2.4);
    }
    return (unsigned short)lrint(r * 4095.0);
}

static double rgb_pwm2srgb(unsigned short n) {
    double r = n / 4095.0;
    if (r <= 0.0031308) {
        r = 12.92 * r;
    } else {
        r = 1.055 * pow(r, 1/2.4) - 0.055;
    }
    return r;
}

RGBAPI bool rgb_pwm_srgb(double r, double g, double b) {
    return rgb_pwm(rgb_srgb2pwm(r), rgb_srgb2pwm(g), rgb_srgb2pwm(b));
}


RGBAPI bool rgb_pwm_srgb256(unsigned char r, unsigned char g, unsigned char b) {
    return rgb_pwm(rgb_srgb2pwm((double)r / 255.0), rgb_srgb2pwm((double)g / 255.0), rgb_srgb2pwm((double)b / 255.0));
}

#define TYPE unsigned short
static TYPE shr(TYPE *n) {
        TYPE c = *n & 1;
        *n >>= 1;
        return c;
}

static void ror(TYPE *n, TYPE *c) {
        TYPE newc = *n & 1;
        *n >>= 1;
        if (*c) *n |= 0x8000;
        *c = newc;
}

// Multiplication using fast integer math
static TYPE mult(TYPE a, TYPE b) {
        TYPE r = 0;
        TYPE c;

        c = shr(&b);
        b += c;

        c = 1;
        while (1) {
                ror (&a, &c);
                if (a == 0) break;
                if (c) {
                        r >>= 1;
                        r += b;         // Max is 7FFF + 8000, so no overflow
                        c = 0;
                } else {
                        c = shr(&r);
//                        r += c;         // Max is 7FFF + 0, so no overflow
                        c = 0;
                }
        }
        return r;
}

static unsigned short multmatch(unsigned short m) {
    long est = lrint(sqrt(m / 4095.0) * 65535.0);
    int err;
    int lerr = 4096;

    if (est > 0xFFFF) est = 0xFFFF;

    while (1) {
        int abserr;

        err = m - (mult(est, est) >> 4);

        abserr = abs(err);
        if (err == 0 || lerr < abserr) {
            return est;
        }
        lerr = abserr;

        if (err > 0) {
            est++;
        } else {
            est--;
        }
    }
}

RGBAPI bool rgb_matchpwm(unsigned short r, unsigned short g, unsigned short b) {
    return rgb_squared(multmatch(r), multmatch(g), multmatch(b));
}

RGBAPI bool rgb_matchpwm_srgb(double r, double g, double b) {
    return rgb_matchpwm(rgb_srgb2pwm(r), rgb_srgb2pwm(g), rgb_srgb2pwm(b));
}

RGBAPI bool rgb_fadeprep(void) {
    unsigned short rgbout[3];

    if (!rgb_getout(rgbout)) return false;
    return rgb_matchpwm(rgbout[0], rgbout[1], rgbout[2]);
}

/*** READING ***/

//unsigned short rgb_initialpwm[3];

// Put triplet from sign into array of 3 unsigned shorts
static void rgb_unpack3(unsigned char *buf, unsigned short *dest) {
    dest[0] = buf[5] + (buf[4] << 8);
    dest[1] = buf[3] + (buf[2] << 8);
    dest[2] = buf[1] + (buf[0] << 8);
}

// Put PWM value into array of 3 unsigned shorts,
// converting 0-0xFFFF to 0-4095
static void rgb_unpack3pwm(unsigned char *buf, unsigned short *dest) {
    rgb_unpack3(buf, dest);
    dest[0] >>= 4;
    dest[1] >>= 4;
    dest[2] >>= 4;
}

// Get PWM values (3 values, 0-4095)
RGBAPI bool rgb_getout(unsigned short *dest) {
    unsigned char buf[6];
    if (serio_putc(SERCMD_R_RGBOUT) < 0) return false;
    serio_flush();
    if (serio_read(buf, 6) != 6) return false;
    rgb_unpack3pwm(buf, dest);
    return true;
}

// Get PWM values converted to sRGB (3 values, 0 to 1.0)
RGBAPI bool rgb_getout_srgb(double *dest) {
    unsigned short rgbout[3];
    if (!rgb_getout(rgbout)) return false;
    dest[0] = rgb_pwm2srgb(rgbout[0]);
    dest[1] = rgb_pwm2srgb(rgbout[1]);
    dest[2] = rgb_pwm2srgb(rgbout[2]);
    return true;
}

// Get 3 values, 0-0xFFFF
RGBAPI bool rgb_getnormal(unsigned char cmd, unsigned short *dest) {
    unsigned char buf[6];
    if (serio_putc(cmd) < 0) return false;
    serio_flush();
    if (serio_read(buf, 6) != 6) return false;
    rgb_unpack3(buf, dest);
    return true;
}

// Get dot correction values (3 values, 0-63)
RGBAPI bool rgb_getdot(unsigned short *dest) {
    unsigned char buf[6];
    if (serio_putc(SERCMD_R_DOT) < 0) return false;
    serio_flush();
    if (serio_read(buf, 6) != 6) return false;
    rgb_unpack3(buf, dest);
    dest[0] >>= 10;
    dest[1] >>= 10;
    dest[2] >>= 10;
    return true;
}


/* Asynchronous reading thread. Not used because serial becomes unstable
 * during full duplex operation.
 */
#undef RGB_ASYNCPOTS
#ifdef RGB_ASYNCPOTS
static HANDLE readingthreadh = NULL;
static bool dopots = false;
// Pot values are placed here by a thread created by rgb_beginpots()
volatile unsigned short rgb_pots[3];
volatile unsigned char rgb_switch;

static unsigned short rgb_unpack1pot(unsigned char *d) {
    return ((*d) << 8) | ((*(d+1)) & 0xF0);
}

static DWORD WINAPI ReadingThread(LPVOID lpParameter) {
    unsigned char buf[6];
    buf[0] = 0;
    while (dopots) {
        buf[1] = buf[0];
        buf[0] = serio_getc();
        if ((buf[0] != buf[1] + 1) &&
            !(buf[0] == 0 && buf[1] == 0xFF)) {
            printf ("%3x", buf[0]);
        }

#if 0
        serio_read(buf, 6);

        while ((buf[5] & 1) != 1 ||
               (buf[3] & 1) != 0 ||
               (buf[1] & 1) != 0) {
            printf("Sync error\n");
            if (!dopots) goto endpots;
            memcpy(buf, &buf[1], 5);
            buf[5] = serio_getc();
        }

        if (buf[5] & RGB_RSW_MID) {
            rgb_switch = RGB_RSW_MID;
        } else {
            rgb_switch = buf[5] & RGB_RSW_UP;
        }

        rgb_pots[2] = rgb_unpack1pot(&buf[0]);
        rgb_pots[1] = rgb_unpack1pot(&buf[2]);
        rgb_pots[0] = rgb_unpack1pot(&buf[4]);

        printf("Pots = %4i, %4i, %4i\n", rgb_pots[0] >> 4, rgb_pots[1] >> 4, rgb_pots[2] >> 4);
#endif
    }
endpots:
    // Discard characters
    while (serio_getc() >= 0) {}

    return 0;
}

void rgb_beginpots(void) {
    dopots = true;
    readingthreadh = CreateThread(NULL, 0 , ReadingThread, NULL, 0, NULL);
    serio_putc(SERCMD_R_POTS);
}

void rgb_endpots(void) {
    dopots = false;
    serio_putc(SERCMD_R_RGBOUT); // Unlike SERCMD_R_POTS, this command won't repeat
    if (readingthreadh != NULL) {
        WaitForSingleObject(readingthreadh, INFINITE);    // Wait for it to quit
        CloseHandle(readingthreadh);
        readingthreadh = NULL;
    }
}
#endif

/*** INITIALIZATION ***/

RGBAPI void rgb_close(void) {
#ifdef WIN32
    if (doasyncpwm) {
        rgb_endasyncpwm();
    }
#endif

#ifdef RGB_ASYNCPOTS
    if (dopots) {
        rgb_endpots();
    }
#endif

    serio_disconnect();
}

RGBAPI bool rgb_open(char *fn) {
    int i;
#ifdef SERIO_ASYNCPOTS
    int c;
    unsigned char buf[16];
    bool wrapped = false;
#endif

    if (serio_connect(fn, 9600) < 0) return false;
    if (serio_setreadtmout(300) < 0) { // Milliseconds
        serio_disconnect();
        return false;
    }

#ifdef SERIO_ASYNCPOTS
    // Send NOPs
    for  (i = 0; i < 6; i++)
        serio_putc(0xFF);
    // Now, the next character is definitely a command

    // It's possible that pot output continues from before
    //if (serio_pending()) {
    //serio_purge();

    // This command stops pot output and gets current RGB value
    serio_putc(SERCMD_R_RGBOUT);

    // Get characters until timeout, keeping last 16 in buffer
    i = 0;
    while (1) {
        c = serio_getc();
        if (c < 0) break;
        buf[i] = c;
        i++;
        if (i >= 16) {
            wrapped = true;
            i = 0;
        }
    }
    // Last 6 characters will be current rgb value

    if (i < 6) {
        if (wrapped) {
            // Move data so it starts at buf[0]
            if (i > 0) {
                memcpy(&buf[6-i], &buf[0], i);
            }
            memcpy(&buf[0], &buf[10+i], 6-i);
        } else {
            // Else something is wrong because there was no response
            printf("Failed to receive rgbout initially\n");
        }
    }

    rgb_unpack3pwm(buf, rgb_initialpwm);
#else
    // Send NOPs
    for  (i = 0; i < 10; i++) {
        if (serio_putc(0xFF) < 0) {
            serio_disconnect();
            return false;
        }
    }

    // Enough NOPs have been sent that any old input must have arrived.

    if (serio_purge(SERIO_PURGEIN)  < 0) {
        serio_disconnect();
        return false;
    }

    return true;
#endif
}
