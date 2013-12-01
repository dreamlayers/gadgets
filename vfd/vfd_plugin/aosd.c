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

#include <glib.h>
#include <audacious/plugin.h>
//#include "aosd_ui.h"
//#include "aosd_osd.h"
//#include "aosd_cfg.h"
#include "aosd_trigger.h"
#include <audacious/i18n.h>

#define VFDPORT "/dev/ttyS0"
#define VISPLUGIN

#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "vfdm.h"

gboolean plugin_is_active = FALSE;

/* ***************** */
/* plug-in functions */

static bool_t
aosd_init ( void )
{
  if (vfdm_init(VFDPORT) != 0) return false;

  plugin_is_active = TRUE;
#if 0
  g_log_set_handler( NULL , G_LOG_LEVEL_WARNING , g_log_default_handler , NULL );

  global_config = aosd_cfg_new();
  aosd_cfg_load( global_config );

  aosd_osd_init( global_config->osd->misc.transparency_mode );

#endif
  aosd_trigger_start();

  return true;
}


static void
aosd_cleanup ( void )
{
  if ( plugin_is_active == TRUE )
  {
    aosd_trigger_stop();

    vfdm_close();

    plugin_is_active = FALSE;
  }

  return;
}


#if 0
void
aosd_configure ( void )
{
  /* create a new configuration object */
  aosd_cfg_t *cfg = aosd_cfg_new();
  /* fill it with information from config file */
  aosd_cfg_load( cfg );
  /* call the configuration UI */
  aosd_ui_configure( cfg );
  /* delete configuration object */
  aosd_cfg_delete( cfg );
  return;
}


void
aosd_about ( void )
{
  aosd_ui_about();
  return;
}
#endif

static void
aosd_render_pcm(const float * pcm, int channels)
{
    gint chan, x;
    unsigned int vu[2];

//    g_static_mutex_lock(&rgb_buf_mutex);

    for (chan = 0; chan < 2; chan++) {
        double d, total = 0;

        for (x = chan; x < (512*2); x+=2) {
          d = pcm[x] * 32768.0;
          d /= 128.0 * 256.0;
          total += d * d;
        }

        if (total == 0) total = 1.0 / 1000000 / 1000000;
        total = sqrt(total);
        total /= 12.0;

        total = log(total) * 20.0 / log(10);

        d = total + 50.0;
        d = d * 14.0 / 50.0;

        vu[chan] = lrint(d);
    }

    vfdm_vu(vu[0], vu[1]);

//    g_static_mutex_unlock(&rgb_buf_mutex);

    return;
}

#if defined(__GNUC__) && __GNUC__ >= 4
#define HAVE_VISIBILITY 1
#else
#define HAVE_VISIBILITY 0
#endif

#if HAVE_VISIBILITY
#pragma GCC visibility push(default)
#endif

#ifdef VISPLUGIN
AUD_VIS_PLUGIN(
#else
AUD_GENERAL_PLUGIN(
#endif
    .name = "Audacious VFD",
    .init = aosd_init,
    //.about = aosd_about,
    //.configure = aosd_configure,
    .cleanup = aosd_cleanup,
#ifdef VISPLUGIN
    .render_multi_pcm = aosd_render_pcm,
#endif
)

#if HAVE_VISIBILITY
#pragma GCC visibility pop
#endif
