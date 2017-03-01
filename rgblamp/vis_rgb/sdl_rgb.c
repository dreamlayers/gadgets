/* Standalone visualization plugin code, using screen instead of RGB lamp. */
/* Copyright 2017 Boris Gjenero. Released under the MIT license. */

#include <stdbool.h>
#include <math.h>
#include <SDL.h>
#include "librgblamp.h"


static unsigned int savedr = 0, savedg = 0, savedb = 0;
static unsigned width, height;
static SDL_Surface *screen = NULL;

static void sdlError(const char *str)
{
    fprintf(stderr, "Error %s\n", str);
    fprintf(stderr,"(reason for error: %s)\n", SDL_GetError());
}

static bool redraw(void) {
    if SDL_MUSTLOCK(screen) {
        SDL_LockSurface(screen);
    }
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,
                                          savedr, savedg, savedb));
    if SDL_MUSTLOCK(screen) {
        SDL_UnlockSurface(screen);
    }
    SDL_UpdateRect(screen, 0, 0, width, height);
    return true; // TODO: error handling
}

static bool screen_init(unsigned int newwidth, unsigned int newheight) {
    width = newwidth;
    height = newheight;

    screen = SDL_SetVideoMode(width, height, 0, SDL_HWSURFACE | SDL_RESIZABLE);

    SDL_WM_SetCaption("RGB lamp visualizer", "RGB lamp visualizer");

    if (!screen) {
        sdlError("setting video mode");
        return false;
    } else {
        return redraw();
    }
}

RGBAPI bool rgb_open(const char *fn) {
    (void)fn;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        sdlError("initializing SDL");
        return false;
    }

    return screen_init(640, 480);
}

static unsigned int pwm2srgb256(unsigned short n) {
    double r = n / 4095.0;

    if (r < 0.0) r = 0.0;
    if (r > 1.0) r = 1.0;

    if (r <= 0.0031308) {
        r = 12.92 * r;
    } else {
        r = 1.055 * pow(r, 1/2.4) - 0.055;
    }
    if (r > 1.0) r = 1.0;
    if (r < 0.0) r = 0.0;
    return r * 255.0;
}

static bool pollevents(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) {
        switch (event.type) {
        case SDL_VIDEOEXPOSE:
            if (!redraw()) return false;
            break;
        case SDL_VIDEORESIZE:
            if (width != (event.resize.w) || height != (event.resize.h)) {
                if (!screen_init(event.resize.w, event.resize.h)) return false;
            }
            break;
        case SDL_QUIT:
            return false;
            break;

        default:
            break;
        }
    }

    return true;
}

RGBAPI bool rgb_pwm(unsigned short r, unsigned short g, unsigned short b) {
    savedr = pwm2srgb256(r);
    savedg = pwm2srgb256(g);
    savedb = pwm2srgb256(b);
    return redraw() && pollevents();
}

RGBAPI void rgb_close(void) {
    SDL_Quit();
}

/* Functions which do nothing and exist to satisfy rgbm.c */
RGBAPI bool rgb_matchpwm(unsigned short r, unsigned short g, unsigned short b) {
    (void)r;
    (void)g;
    (void)b;
    return true;
}

RGBAPI bool rgb_flush(void) {
    return true;
}
