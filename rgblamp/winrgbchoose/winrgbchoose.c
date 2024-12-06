/* Windows colour chooser for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <windows.h>
#include <stdio.h>
#ifdef STANDALONE_WRGBC
#include "librgblamp.h"
#endif
#ifdef COLORANIMD_WRGBC
#include "coloranim.h"
#include "signd.h"
#endif

static struct {
    int r;
    int g;
    int b;
} rgb;

static BOOL rgb_changed = FALSE;

static void update_color(int *was, int new) {
    if (*was != new) {
        *was = new;
        rgb_changed = TRUE;
    }
}

static UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg,
                                    WPARAM wParam, LPARAM lParam) {
    char buf[4];
    int val;

    //printf("%x %s (%x) %x %x\n", hdlg, wm2text(uiMsg), uiMsg, wParam, lParam);
    if (uiMsg == WM_COMMAND && HIWORD(wParam) == EN_CHANGE) {
        SendMessage((HWND)lParam, WM_GETTEXT,
                    sizeof(buf)/sizeof(buf[0]), (LPARAM)buf);
        //printf("%x = %s\n", LOWORD(wParam), buf);
        val = atoi(buf);
        switch (LOWORD(wParam)) {
        case 0x2c2:
            update_color(&rgb.r, val); break;
        case 0x2c3:
            update_color(&rgb.g, val); break;
        case 0x2c4:
            update_color(&rgb.b, val);
            if (rgb_changed) {
#ifdef STANDALONE_WRGBC
                rgb_pwm_srgb256(rgb.r, rgb.g, rgb.b);
#endif
#ifdef COLORANIMD_WRGBC
                char buf[100];
                int l;
                l = snprintf(buf, sizeof(buf), "in 0.25 %f %f %f",
                             rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0);
                cmd_enq_string(2, buf, l);
#endif
            }
        }
    }
    return 0;
}

static BOOL wingetcolor(void) {
    CHOOSECOLOR cc;
    COLORREF custcol[15];
    BOOL r;

    memset(&cc, 0, sizeof(cc));
    memset(&custcol, 0, sizeof(custcol));
    cc.lStructSize = sizeof(cc);
    cc.lpCustColors = custcol;
    cc.rgbResult = RGB(rgb.r, rgb.g, rgb.b);
    cc.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;
    cc.lpfnHook = CCHookProc;
    cc.Flags |= CC_ENABLEHOOK;
    r = ChooseColor(&cc);
    if (r) {
        rgb.r = GetRValue(cc.rgbResult);
        rgb.g = GetGValue(cc.rgbResult);
        rgb.b = GetBValue(cc.rgbResult);
    }
    return r;
}

#ifdef STANDALONE_WRGBC
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
    const char *port =
#ifndef RGBPORT
        "COM8";
#else
        RGBPORT;
#endif
    unsigned char oldrgb[3];

    if (lpCmdLine != NULL && lpCmdLine[0] != 0) port = lpCmdLine;
    if (!rgb_open(port)) return -1;

    if (!rgb_getout_srgb256(oldrgb)) return -1;
    rgb.r = oldrgb[0];
    rgb.g = oldrgb[1];
    rgb.b = oldrgb[2];

    wingetcolor();

    rgb_matchpwm_srgb256(rgb.r, rgb.g, rgb.b);
    rgb_close();

    return 0;
}
#endif

#ifdef COLORANIMD_WRGBC
void signd_icondblclick(HWND hwnd) {
    rgb.r = 0;
    rgb.g = 0;
    rgb.b = 0;
    wingetcolor();
}
#endif
