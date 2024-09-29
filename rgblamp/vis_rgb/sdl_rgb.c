/* Standalone visualization plugin code, using screen instead of RGB lamp. */
/* Copyright 2017 Boris Gjenero. Released under the MIT license. */

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <SDL.h>
#include "rgbm.h"

#define TITLE "RGB lamp visualizer"
#define DEFAULT_WIDTH (640)
#define DEFAULT_HEIGHT (480)

static unsigned int savedr = 0, savedg = 0, savedb = 0;
#if !SDL_VERSION_ATLEAST(2,0,0)
static unsigned int winwidth = DEFAULT_WIDTH, winheight = DEFAULT_HEIGHT;
static unsigned int nativewidth = 0, nativeheight = 0;
#endif
static bool fullscreen = false;

#if SDL_VERSION_ATLEAST(2,0,0)
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
#else
static SDL_Surface *screen = NULL;
#endif

static void sdlError(const char *str)
{
    fprintf(stderr, "Error %s\n", str);
    fprintf(stderr,"(reason for error: %s)\n", SDL_GetError());
}

static bool redraw(void) {
#if SDL_VERSION_ATLEAST(2,0,0)
    if (SDL_SetRenderDrawColor(renderer, savedr, savedg, savedb,
                                SDL_ALPHA_OPAQUE) ||
        SDL_RenderClear(renderer))
        return false;
    SDL_RenderPresent(renderer);
#else
    if SDL_MUSTLOCK(screen) {
        SDL_LockSurface(screen);
    }
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,
                                          savedr, savedg, savedb));
    if SDL_MUSTLOCK(screen) {
        SDL_UnlockSurface(screen);
    }
    SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
     // TODO: SDL 1 error handling
#endif
    return true;
}

static bool screen_init(unsigned int newwidth, unsigned int newheight) {
#if SDL_VERSION_ATLEAST(2,0,0)
    window = SDL_CreateWindow(TITLE,
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              newwidth, newheight,
                              newwidth ? SDL_WINDOW_RESIZABLE :
                                         SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (window) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }
    if (!window || !renderer)
#else
    Uint32 flags = SDL_HWSURFACE;
    if (newwidth == 0) {
        if (screen && !fullscreen) {
            winwidth = screen->w;
            winheight = screen->h;
        }
        newwidth = nativewidth;
        newheight = nativeheight;
        flags |= SDL_FULLSCREEN;
        fullscreen = true;
    } else {
        flags |= SDL_RESIZABLE;
        fullscreen = false;
    }

    screen = SDL_SetVideoMode(newwidth, newheight, 0, flags);
    SDL_WM_SetCaption(TITLE, TITLE);
    if (!screen)
#endif
    {
        sdlError("setting video mode");
        return false;
    } else {
        return redraw();
    }
}

int rgbm_hw_open(void) {
#if !SDL_VERSION_ATLEAST(2,0,0)
    const SDL_VideoInfo *vi;
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        sdlError("initializing SDL");
        return false;
    }

#if !SDL_VERSION_ATLEAST(2,0,0)
    vi = SDL_GetVideoInfo();
    if (vi != NULL) {
        nativewidth = vi->current_w;
        nativeheight = vi->current_h;
    }
#endif

    return screen_init(DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

#ifndef RGBM_AGCUP
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
#else /* RGBM_AGCUP */
static unsigned int srgb2srgb256(double r) {
    if (r < 0.0) r = 0.0;
    if (r > 1.0) r = 1.0;

    return r * 255.0;
}
#endif

static bool pollevents(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0) {
        switch (event.type) {
#if SDL_VERSION_ATLEAST(2,0,0)
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                if (!redraw()) return false;
            }
            break;
#else
        case SDL_VIDEOEXPOSE:
            if (!redraw()) return false;
            break;
        case SDL_VIDEORESIZE:
            if (screen->w != (event.resize.w) ||
                screen->h != (event.resize.h)) {
                if (!screen_init(event.resize.w, event.resize.h)) return false;
            }
            break;
#endif
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT
#if SDL_VERSION_ATLEAST(2,0,2)
                 && event.button.clicks == 2
#endif
                ) {
                bool res;
                if (!fullscreen) {
#if SDL_VERSION_ATLEAST(2,0,0)
                    res = !SDL_SetWindowFullscreen(window,
                                                   SDL_WINDOW_FULLSCREEN_DESKTOP);
                    fullscreen = true;
#else
                    res = screen_init(0, 0);
#endif
                } else {
#if SDL_VERSION_ATLEAST(2,0,0)
                    res = !SDL_SetWindowFullscreen(window, 0);
                    fullscreen = false;
#else
                    res = screen_init(winwidth, winheight);
#endif
                }
                if (!res) {
                    sdlError("toggling full screen size");
                    return false;
                }
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

#ifndef RGBM_AGCUP
int rgbm_hw_pwm(const double *rgb) {
    savedr = pwm2srgb256(rgb[0]);
    savedg = pwm2srgb256(rgb[1]);
    savedb = pwm2srgb256(rgb[2]);
    return redraw() && pollevents();
}
#else /* RGBM_AGCUP */
int rgbm_hw_srgb(const double *rgb) {
    savedr = srgb2srgb256(rgb[0]);
    savedg = srgb2srgb256(rgb[1]);
    savedb = srgb2srgb256(rgb[2]);
    return redraw() && pollevents();
}
#endif

void rgbm_hw_close(void) {
    SDL_Quit();
}
