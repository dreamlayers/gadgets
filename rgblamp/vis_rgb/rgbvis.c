// Winamp test dsp library 0.9 for Winamp 2
// Copyright (C) 1997, Justin Frankel/Nullsoft
// Feel free to base any plugins on this "framework"...

#define RGBPORT "COM8"

#define __W32API_USE_DLLIMPORT__
#define WIN32_LEAN_AND_MEAN
#include "librgblamp.h"
#include <math.h>
#include <windows.h>
#include "vis.h"
#define MODULETYPE winampVisModule

/***** Global Variables *****/

static bool noerror = false;
static unsigned long int movavg[3];

/***** Winamp module functions *****/

static void config(struct MODULETYPE *this_mod) {
    MessageBox(this_mod->hwndParent,"This module is Copyright(C) 2011, Boris Gjenero\n",
                                    "Configuration",MB_OK);
}

static int init(struct MODULETYPE *this_mod) {
    noerror = rgb_open(RGBPORT);
    if (noerror) {
        return 0;
    } else {
        MessageBox(this_mod->hwndParent,"Cannot connect to lamp\n",
                                        "RGB Lamp Visualization",MB_OK);
        return 1;
    }
}

static void quit(struct MODULETYPE *this_mod) {
    if (noerror) {
        rgb_matchpwm((unsigned short)movavg[0],
                     (unsigned short)movavg[1],
                     (unsigned short)movavg[2]);
    }
    rgb_close();
}

static int render(struct winampVisModule *this_mod) {
    unsigned int colour, x;
    static const int bounds[] = { 0, 14, 60, 288 };
    #define MASIZE 16

    for (colour = 0; colour < 3; colour++) {
        unsigned int bound = bounds[colour+1];
        unsigned long sum = 0;
        for (x = bounds[colour]; x < bound; x++) {
            sum += this_mod->spectrumData[0][x];
        }
        sum = sum * 4095 / (bound - bounds[colour]) / 255;
        movavg[colour] = (movavg[colour] * (MASIZE - 1) + sum) / MASIZE;
    }

    noerror = rgb_pwm((unsigned short)movavg[0],
                      (unsigned short)movavg[1],
                      (unsigned short)movavg[2]);
    return noerror ? 0 : 1;
}

/***** Winamp module interface *****/

// third module (VU meter)
static winampVisModule mod = {
    "VU Meter",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    25,     // latencyMS
    25,     // delayMS
    1,      // spectrumNch
    0,      // waveformNch
    { { 0, 0 }, }, // spectrumData
    { { 0, 0 }, }, // waveformData
    config,
    init,
    render,
    quit
};

static MODULETYPE *getModule(int which) {
    switch (which) {
        case 0:  return &mod;
        default: return NULL;
    }
}

// Module header, includes version, description, and address of the module retriever function
static winampVisHeader hdr = { VIS_HDRVER, "RGB Lamp Visualization", getModule };

#ifdef __cplusplus
extern "C" {
#endif
// this is the only exported symbol. returns our main header.
__declspec( dllexport ) winampVisHeader *winampVisGetHeader(void);
__declspec( dllexport ) winampVisHeader *winampVisGetHeader(void) {
    return &hdr;
}
#ifdef __cplusplus
}
#endif
