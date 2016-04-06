/*
*
* Author: Giacomo Lozito <james@develia.org>, (C) 2005-2007
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*
*/

#include <math.h>
#include <string.h>
#include <glib.h>
#include <libaudcore/i18n.h>
#define AUD_PLUGIN_GLIB_ONLY
#include <libaudcore/plugin.h>
#include "avfd_trigger.h"

#include "vfdm.h"

#define VFDPORT "/dev/ttyS0"

class AVFD : public VisPlugin
{
public:
    static const char about[];

    static constexpr PluginInfo info = {
        N_("Audacious VFD"),
        N_("gadgets"),
        about
    };

    constexpr AVFD () : VisPlugin (info, Visualizer::MultiPCM) {}

    bool init ();
    void cleanup ();

    void render_multi_pcm (const float * pcm, int channels);
    void clear ();
};


__attribute__((visibility("default"))) AVFD aud_plugin_instance;

gboolean plugin_is_active = FALSE;

const char AVFD::about[] =
 N_("Audacious VFD\n");


/* ***************** */
/* plug-in functions */

bool AVFD::init ()
{
  if (vfdm_init(VFDPORT) != 0) return false;

  plugin_is_active = TRUE;

  aosd_trigger_start();

  return true;
}


void AVFD::cleanup ()
{
  if ( plugin_is_active == TRUE )
  {
    aosd_trigger_stop();

    vfdm_close();

    plugin_is_active = FALSE;
  }

  return;
}

void AVFD::render_multi_pcm(const float * pcm, int channels)
{
    unsigned int vu[2];
    unsigned int chan, usech, x, xlmt;

    if (channels >= 2) {
        usech = 2;
    } else if (channels == 1) {
        usech = 1;
    } else {
        return;
    }

//    g_static_mutex_lock(&rgb_buf_mutex);
    xlmt = 512 * channels;
    for (chan = 0; chan < usech; chan++) {
        double d, total = 0;

        for (x = chan; x < xlmt; x+=channels) {
          d = pcm[x] * 32768.0;
          d /= 128.0 * 256.0;
          total += d * d;
        }

        if (total > 0) {
            total = sqrt(total);
            total /= 12.0;

            total = log(total) * 20.0 / log(10);

            d = total + 50.0;
            d = d * 14.0 / 50.0;

            vu[chan] = lrint(d);
        } else {
            vu[chan] = 0;
        }
    }

    if (usech == 1) vu[1] = vu[0];

    vfdm_vu(vu[0], vu[1]);

//    g_static_mutex_unlock(&rgb_buf_mutex);

    return;
}

void AVFD::clear ()
{
    vfdm_vu(0, 0);
}
