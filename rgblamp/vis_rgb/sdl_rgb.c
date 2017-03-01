/* Standalone visualization plugin code, using screen instead of RGB lamp. */
/* Copyright 2017 Boris Gjenero. Released under the MIT license. */

#include <stdbool.h>
#include <math.h>
#include <SDL.h>
#include "librgblamp.h"

#define width 640
#define height 480

static void sdlError(const char *str)
{
    fprintf(stderr, "Error %s\n", str);
    fprintf(stderr,"(reason for error: %s)\n", SDL_GetError());
}

static SDL_Surface *screen;

RGBAPI bool rgb_open(const char *fn) {
    (void)fn;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        sdlError("initializing SDL");
        return false;
    }

    SDL_WM_SetCaption("RGB lamp visualizer", "RGB lamp visualizer");

    screen = SDL_SetVideoMode(width, height, 0, SDL_HWSURFACE);

    if (!screen) {
        sdlError("setting video mode");
        return false;
    }

    return true;
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
    return SDL_PollEvent(&event) != 0 && event.type == SDL_QUIT;
}

RGBAPI bool rgb_pwm(unsigned short r, unsigned short g, unsigned short b) {
    if SDL_MUSTLOCK(screen) {
        SDL_LockSurface(screen);
    }
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,
                                          pwm2srgb256(r),
                                          pwm2srgb256(g),
                                          pwm2srgb256(b)));
    if SDL_MUSTLOCK(screen) {
        SDL_UnlockSurface(screen);
    }
    SDL_UpdateRect(screen, 0, 0, width, height);
    return !pollevents();
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
