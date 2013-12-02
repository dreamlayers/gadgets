#define VFDPORT "COM1"
#define SPECTRUMVU
#if !defined(DSPPLUG) && !defined(VISPLUG)
//#define DSPPLUG
#define VISPLUG
#endif

#define __W32API_USE_DLLIMPORT__
#define WIN32_LEAN_AND_MEAN
#ifndef _MSC_VER
#include <stdbool.h>
#else
#define bool int
#define true 1
#define false 0
#endif
#include <stdio.h>
#include <math.h>
#ifdef _MSC_VER
#include "lrint.h"
#endif
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>
#if defined(DSPPLUG)
#include "dsp.h"
#define MODULETYPE winampDSPModule
#elif defined(VISPLUG)
#include "vis.h"
#define MODULETYPE winampVisModule
#endif
#include "vfdm.h"

/***** Winamp module interface *****/

#define IPC_ISPLAYING 104
#define WM_WA_IPC WM_USER

static MODULETYPE *getModule(int which);

static void config(struct MODULETYPE *this_mod);
static int init(struct MODULETYPE *this_mod);
static void quit(struct MODULETYPE *this_mod);
#if defined(DSPPLUG)
static int modify_samples1(struct MODULETYPE *this_mod, short int *samples, int numsamples, int bps, int nch, int srate);
#elif defined(VISPLUG)
static int render(struct winampVisModule *this_mod);
#endif

#if defined(DSPPLUG)
// Module header, includes version, description, and address of the module retriever function
static winampDSPHeader hdr = { DSP_HDRVER, "VFD DSP Plugin", getModule };

// first module
static winampDSPModule mod = {
    "VFD Plugin",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    config,
    init,
    modify_samples1,
    quit
};
#elif defined(VISPLUG)
// Module header, includes version, description, and address of the module retriever function
static winampVisHeader hdr = { VIS_HDRVER, "VFD Visualization Plugin", getModule };

// third module (VU meter)
static winampVisModule mod =
{
    "VU Meter",
    NULL,   // hwndParent
    NULL,   // hDllInstance
    0,      // sRate
    0,      // nCh
    25,     // latencyMS
    25,     // delayMS
#ifdef SPECTRUMVU
    2,      // spectrumNch
    0,      // waveformNch
#else
    0,      // spectrumNch
    2,      // waveformNch
#endif
    { { 0, 0 } }, // spectrumData
    { { 0, 0 } }, // waveformData
    config,
    init,
    render,
    quit
};
#else
#error Define DSPPLUG or VISPLUG to determine type of plugin to build
#endif


#ifdef __cplusplus
extern "C" {
#endif
// this is the only exported symbol. returns our main header.
// No prototype, so add one here to silence GCC warning.
#if defined(DSPPLUG)
__declspec( dllexport ) winampDSPHeader *winampDSPGetHeader2(void);
__declspec( dllexport ) winampDSPHeader *winampDSPGetHeader2(void)
#elif defined(VISPLUG)
__declspec( dllexport ) winampVisHeader *winampVisGetHeader(void);
__declspec( dllexport ) winampVisHeader *winampVisGetHeader(void)
#endif
{
    return &hdr;
}
#ifdef __cplusplus
}
#endif

static MODULETYPE *getModule(int which) {
    switch (which) {
        case 0:  return &mod;
        default: return NULL;
    }
}

/***** Global Variables *****/

static HHOOK WAHook = NULL;

/***** Functions for the Winamp event loop ******/

/* Find the "- Winamp" suffix in the Winamp title */
static char *findwa(char *s) {
    char *cand;
    register char *p;
    const char *waname = "- winamp";
    register const char *p2;

    cand = NULL;
    p = s;
    p2 = waname;

    while (*p != 0) {
        if (tolower(*p) != *p2) {
            p2 = waname;
        } else {
            p2++;
            if (*p2 == 0) {
                /* Found match, store location of '-'. */
                cand = p - 7;
                p2 = waname;
                /* Keep scanning, because we need the last match */
            }
        }
        p++;
    }

    if (cand == NULL) {
        /* Not found. Return end of string */
        return p;
    } else  {
        /* Skip over spaces before "- Winamp" */
        p = cand;
        while (p > s && *(--p) == ' ') { }

        return p + 1;
    }
}

/* If track name changed, store it and tell VFD thread to update */
static void updtrkname(char *s) {
    char *p, *trkp;
    int l, trknum;

    p = findwa(s);
    l = p - s;

    trknum = strtol(s, &trkp, 10); /* Extract and skip track number */
    l -= (trkp - s);
    /* This leaves Winamp ". " as separator at beginning/end */

    /* No spaces at end */
    if (l > VFD_SCROLLMAX) l = VFD_SCROLLMAX;
    while (l > 0 && trkp[l-1] == ' ') l--;

    vfdm_trackchange(trknum, trkp, l);
}

/* Winamp window hook for obtaining updates */
static LRESULT WINAPI HookWinampWnd(int nCode, WPARAM wParam, LPARAM lParam) {
    /* The useful parameters are:
     * ((CWPRETSTRUCT *)lParam)->hwnd, ((CWPRETSTRUCT *)lParam)->message,
     * ((CWPRETSTRUCT *)lParam)->wParam, ((CWPRETSTRUCT *)lParam)->lParam)
     */

    /* Process only if HC_ACTION according to MS docs. */
    if (nCode == HC_ACTION && ((CWPRETSTRUCT *)lParam)->hwnd == mod.hwndParent) {
        switch(((CWPRETSTRUCT *)lParam)->message) {
        /* Begin power management */
        case WM_POWERBROADCAST:
            if (((CWPRETSTRUCT *)lParam)->wParam == 4 /* PBT_APMSUSPEND */) {
                vfdm_pmsuspend();
            } else if (((CWPRETSTRUCT *)lParam)->wParam == 18 /* PBT_APMRESUMEAUTOMATIC */) {
                vfdm_pmwake();
            }
            break;
        /* End power management */

        case WM_SETTEXT:
            updtrkname((char *)(((CWPRETSTRUCT *)lParam)->lParam));
            break;

        case WM_COMMAND:
            switch (((CWPRETSTRUCT *)lParam)->wParam) {
            case 40045:
            case 40046:
            case 40047: vfdm_playstatechanged(); break;
            }
            break;
#if 0
        case WM_VFDPAR: if ((((CWPRETSTRUCT *)lParam)->lParam) == WM_VFDPAR_COOKIE) {
            int temp = (parq_head + 1) % PARQLEN;
            if (temp != parq_tail) {
                parq[parq_head] = (((CWPRETSTRUCT *)lParam)->wParam) & 0xFF;
                parq_head = temp;
            }
            SetEvent(VFDtodo);
        }
        break;
#endif

        default:
          break;
      }


    }

    return CallNextHookEx(WAHook, nCode,
        wParam, lParam);
}

/***** Helper functions for VFD thread *****/

/* Get new state from Winamp and update indicators if the state changed */
enum vfdm_playstate vfdm_cb_getplaystate(void) {
    return (enum vfdm_playstate)SendMessage(mod.hwndParent,WM_WA_IPC,0,IPC_ISPLAYING);
}

/***** Winamp module functions *****/

static void config(struct MODULETYPE *this_mod) {
    MessageBox(this_mod->hwndParent,"VFD Winamp plugin built on "__DATE__", "__TIME__,
                                    "Configuration",MB_OK);
}

static int init(struct MODULETYPE *this_mod) {
    char buf[256];

    vfdm_init(VFDPORT);

    /* Initialize variables used for communication with thread */
    GetWindowText(mod.hwndParent, buf, sizeof(buf));
    updtrkname(buf);

    WAHook = SetWindowsHookEx(WH_CALLWNDPROCRET, HookWinampWnd, NULL,
                              GetWindowThreadProcessId(mod.hwndParent, NULL));

    return 0;
}

// cleanup (opposite of init()). Destroys the window, unregisters the window class
static void quit(struct MODULETYPE *this_mod) {
    if (WAHook != NULL) UnhookWindowsHookEx(WAHook);
    vfdm_close();
}

#if defined(DSPPLUG)
static int modify_samples1(struct winampDSPModule *this_mod, short int *samples, int numsamples, int bps, int nch, int srate)
#elif defined(VISPLUG)
static int render(struct winampVisModule *this_mod)
#endif
{
  int x, y, usech;
  unsigned int vu[2];

#if defined(DSPPLUG)
  usech = (nch > 2) ? 2 : nch;

  if (bps == 16 || bps == 24) {
#elif defined(VISPLUG)
  usech = 2;
#endif

    for (y = 0; y < usech; y ++) {
      double total=0;
      double d = 0;

#if defined(DSPPLUG)
      if (bps == 16) {
        for (x = 1; x < numsamples; x ++) {
          d = ((short int *)samples)[x * nch + y];
          d /= 128.0 * 256.0;
          total += d * d;
        }
      } else if (bps == 24) {
        for (x = 1; x < numsamples; x ++) {
          d = *((int *)((char *)samples + x * nch * 3 + y * 3 - 1)) >> 8;
          d /= 128.0 * 256.0 * 256.0;
          total += d * d;
        }
      }
      total /= (double)numsamples;
#elif defined(VISPLUG) && defined(SPECTRUMVU)
      /* New code using spectrum */
      for (x = 0; x < 576; x++) {
        d = (double)this_mod->spectrumData[y][x] / 255.0;
        // d = d * d;
        // total += spect;
        if (d > total) total = d;
      }
      //total = sqrt(total);
      vu[y] = (unsigned int)(total * 14.0 + 0.5);
#elif defined(VISPLUG)
      for (x = 1; x < 576; x ++) {
        d = (signed char)(this_mod->waveformData[y][x]);
        d /= 128.0;
        total += d * d;
      }
      total /= 576.0;
#endif

#if !defined(VISPLUG) || !defined(SPECTRUMVU)
      if (total == 0) total = 1.0 / 1000000 / 1000000;
      total = sqrt(total);
      total = log(total) * 20.0 / log(10.0);

      d = total + 50.0;
      d = d * 14.0 / 50.0;

      vu[y] = lrint(d);
#endif
      if (vu[y] < 0) vu[y] = 0;
      if (vu[y] > 14) vu[y] = 14;
    }
#if defined(DSPPLUG)
  }

  if (nch == 1) vu[1] = vu[0];
#endif

  vfdm_vu(vu[0], vu[1]);

#if defined(DSPPLUG)
  return numsamples;
#elif defined(VISPLUG)
  return 0;
#endif
}
