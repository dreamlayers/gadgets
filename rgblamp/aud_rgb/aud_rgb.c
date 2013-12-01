/*  BMP - Cross-platform multimedia player
 *  Copyright (C) 2003-2004  BMP development team.
 *
 *  Based on XMMS:
 *  Copyright (C) 1998-2003  XMMS development team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <math.h>
#include <glib.h>

#include <audacious/misc.h>
#include <audacious/plugin.h>

#include "librgblamp.h"

#define RGBPORT "/dev/ttyUSB0"

static bool noerror = false;
static double srgb[3];

static int
rgblamp_init(void)
{
    noerror = rgb_open(RGBPORT);
    return noerror;
}

static void
rgblamp_cleanup(void)
{
    rgb_matchpwm_srgb(srgb[0], srgb[1], srgb[2]);
    rgb_close();
}

#if 0
static void
rgblamp_playback_stop(void)
{
}
#endif

static inline void
update_ema(double *ema, double m, double s)
{
    *ema = (*ema * (m-1) + s) / m;
}

static void
rgblamp_render_freq(const gfloat *freq)
{
    unsigned int colour, x;
    static const int bounds[] = { 0, 14, 45, 512 };
    #define MASHORT 16
    #define MALONGUP 512
    #define MALONGDN 128
    static double mashort[3] = { 0, 0, 0 };
    static double malong[3] = { 0.001, 0.001, 0.001 };

    for (colour = 0; colour < 3; colour++) {
        const unsigned int bound = bounds[colour+1];
        double sum = 0;
        for (x = bounds[colour]; x < bound; x++) {
            sum += freq[x] * freq[x];
            //printf ("%f ", freq[x]);
        }
        sum = sqrt(sum / (bound - bounds[colour]));

        update_ema(&mashort[colour], MASHORT, sum);
        update_ema(&malong[colour],
                    malong[colour] > sum ? MALONGDN : MALONGUP,
                    sum);

        srgb[colour] = (mashort[colour]/(malong[colour] + 0.000001)) / 8;
    }
    //printf("\n");

    rgb_pwm_srgb(srgb[0], srgb[1], srgb[2]);
}

#if defined(__GNUC__) && __GNUC__ >= 4
#define HAVE_VISIBILITY 1
#else
#define HAVE_VISIBILITY 0
#endif

#if HAVE_VISIBILITY
#pragma GCC visibility push(default)
#endif

AUD_VIS_PLUGIN(
    .name = "RGB Lamp",
    .init = rgblamp_init,
    .cleanup = rgblamp_cleanup,
    //.about = aosd_about,
    //.configure = rgblamp_configure,
    .render_freq = rgblamp_render_freq,
)

#if HAVE_VISIBILITY
#pragma GCC visibility pop
#endif
