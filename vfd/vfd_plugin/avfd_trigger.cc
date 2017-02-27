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

//#define DEBUG
#ifdef DEBUG
#include <stdio.h>
#endif

#include <glib.h>

#include <libaudcore/i18n.h>
#include <libaudcore/drct.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugin.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>

#include "avfd_trigger.h"
#include "avfd_trigger_private.h"

#include <string.h>
#include "vfdm.h"

enum vfdm_playstate playstate = VFDM_UNKNOWNSTATE;

enum vfdm_playstate vfdm_cb_getplaystate(void) {
    return playstate;
}

static void updatestate(enum vfdm_playstate newstate) {
#ifdef DEBUG
    printf("updatestate: %i\n", newstate);
#endif
    playstate = newstate;
    vfdm_playstatechanged();
}

static void audvfd_settitle(const gchar *title) {
    gchar buf[VFD_SCROLLMAX];
    int len;

    /* Skip track number and ". " after it */
    const gchar *p = title;
    while (*p >= '0' && *p <= '9') p++;
    if (*p == '.') p++;
    if (*p == ' ') p++;

    len = strlen(p);

    /* Remove track time */
    const gchar *p2 = &p[len-1];
    while (p2 >= p &&
           (*p2 == '(' || *p2 == ')' || *p2 == ':' ||
            (*p2 >= '0' && *p2 <= '9'))) p2--;
    if (p2 >= p && *p2 == ' ') p2--;
    len = p2 - p + 1;

    /* Limit length, append ". " as separator */
    if (len > VFD_SCROLLMAX-2) len = VFD_SCROLLMAX-2;
    strncpy(buf, p, len);
    buf[len] = '.';
    buf[len+1] = ' ';

    vfdm_trackchange(0 /* FIXME track # */, buf, len+2);
}


/* trigger codes ( the code values do not need to be sequential ) */
enum
{
  AOSD_TRIGGER_PB_START = 0,
  AOSD_TRIGGER_PB_TITLECHANGE,
  AOSD_TRIGGER_PB_PAUSEON,
  AOSD_TRIGGER_PB_PAUSEOFF,
  AOSD_TRIGGER_PB_STOP
};

/* prototypes of trigger functions */
static void aosd_trigger_func_pb_start_onoff ( gboolean );
static void aosd_trigger_func_pb_start_cb ( gpointer , gpointer );
static void aosd_trigger_func_pb_titlechange_onoff ( gboolean );
static void aosd_trigger_func_pb_titlechange_cb ( gpointer , gpointer );
static void aosd_trigger_func_pb_pauseon_onoff ( gboolean );
static void aosd_trigger_func_pb_pauseon_cb ( gpointer , gpointer );
static void aosd_trigger_func_pb_pauseoff_onoff ( gboolean );
static void aosd_trigger_func_pb_pauseoff_cb ( gpointer , gpointer );
static void aosd_trigger_func_pb_stop_onoff ( gboolean );
static void aosd_trigger_func_pb_stop_cb ( gpointer , gpointer );
//static void aosd_trigger_func_hook_cb ( gpointer markup_text , gpointer unused );

/* map trigger codes to trigger objects */
aosd_trigger_t aosd_triggers[] =
{
  [AOSD_TRIGGER_PB_START] = { aosd_trigger_func_pb_start_onoff ,
                              aosd_trigger_func_pb_start_cb },

  [AOSD_TRIGGER_PB_TITLECHANGE] = { aosd_trigger_func_pb_titlechange_onoff ,
                                    aosd_trigger_func_pb_titlechange_cb },

  [AOSD_TRIGGER_PB_PAUSEON] = { aosd_trigger_func_pb_pauseon_onoff ,
                                aosd_trigger_func_pb_pauseon_cb },

  [AOSD_TRIGGER_PB_PAUSEOFF] = { aosd_trigger_func_pb_pauseoff_onoff ,
                                 aosd_trigger_func_pb_pauseoff_cb },

  [AOSD_TRIGGER_PB_STOP] = { aosd_trigger_func_pb_stop_onoff ,
                             aosd_trigger_func_pb_stop_cb }
};



/* TRIGGER API */

void
aosd_trigger_start ( void )
{
  for (unsigned int i = 0 ; i < sizeof(aosd_triggers)/sizeof(aosd_triggers[0]) ; i++ )
  {
    aosd_triggers[i].onoff_func( TRUE );
  }

  return;
}


void
aosd_trigger_stop ( void )
{
  for (unsigned int i = 0 ; i < sizeof(aosd_triggers)/sizeof(aosd_triggers[0]) ; i++ )
  {
    aosd_triggers[i].onoff_func( FALSE );
  }
  return;
}


/* HELPER FUNCTIONS */

#if 0
static gchar * aosd_trigger_utf8convert (const gchar * str)
{
  if ( global_config->osd->text.utf8conv_disable == FALSE )
    return str_to_utf8( str );
  else
    return g_strdup( str );
}
#endif


/* TRIGGER FUNCTIONS */

static void
aosd_trigger_func_pb_start_onoff(gboolean turn_on)
{
  if (turn_on == TRUE)
    hook_associate("playback ready", aosd_trigger_func_pb_start_cb, NULL);
  else
    hook_dissociate("playback ready", aosd_trigger_func_pb_start_cb);
  return;
}

static void
aosd_trigger_func_pb_start_cb(gpointer hook_data, gpointer user_data)
{
    String title = aud_drct_get_title ();
#ifdef DEBUG
    if (!title) {
        printf("Start with no title\n");
    } else {
        printf("Start: %s\n", (const char *)title);
    }
#endif

    updatestate(VFDM_PLAYING);

    if (title) audvfd_settitle((const char *)title);

    return;
}

typedef struct
{
  gchar *title;
  gchar *filename;
}
aosd_pb_titlechange_prevs_t;

static void
aosd_trigger_func_pb_titlechange_onoff ( gboolean turn_on )
{
  static aosd_pb_titlechange_prevs_t *prevs = NULL;

  if ( turn_on == TRUE )
  {
    prevs = (aosd_pb_titlechange_prevs_t *)g_malloc0(sizeof(aosd_pb_titlechange_prevs_t));
    prevs->title = NULL;
    prevs->filename = NULL;
    hook_associate( "title change" , aosd_trigger_func_pb_titlechange_cb , prevs );
  }
  else
  {
    hook_dissociate( "title change" , aosd_trigger_func_pb_titlechange_cb );
    if ( prevs != NULL )
    {
      if ( prevs->title != NULL ) g_free( prevs->title );
      if ( prevs->filename != NULL ) g_free( prevs->filename );
      g_free( prevs );
      prevs = NULL;
    }
  }
  return;
}

static void
aosd_trigger_func_pb_titlechange_cb ( gpointer plentry_gp , gpointer prevs_gp )
{
#ifdef DEBUG
  printf("titlechange_cb");
#endif
  if (aud_drct_get_playing ())
  {
    aosd_pb_titlechange_prevs_t *prevs = (aosd_pb_titlechange_prevs_t *)prevs_gp;   
    String pl_entry_filename = aud_drct_get_filename ();
    Tuple pl_entry_tuple = aud_drct_get_tuple ();
    String pl_entry_title = pl_entry_tuple.get_str (Tuple::FormattedTitle);

    /* same filename but title changed, useful to detect http stream song changes */

    if ( ( prevs->title != NULL ) && ( prevs->filename != NULL ) )
    {
      if ( ( pl_entry_filename != NULL ) && ( !strcmp(pl_entry_filename,prevs->filename) ) )
      {
        if ( ( pl_entry_title != NULL ) && ( strcmp(pl_entry_title,prevs->title) ) )
        {
#ifdef DEBUG
          printf("Titlechange: %s\n", static_cast<const char*>(pl_entry_title));
#endif
          audvfd_settitle(pl_entry_title);
        }
      }
      else
      {
        g_free(prevs->filename);
        prevs->filename = g_strdup(pl_entry_filename);
        /* if filename changes, reset title as well */
        if ( prevs->title != NULL )
          g_free(prevs->title);
        prevs->title = g_strdup(pl_entry_title);
      }
    }
    else
    {
      if ( prevs->title != NULL )
        g_free(prevs->title);
      prevs->title = g_strdup(pl_entry_title);
      if ( prevs->filename != NULL )
        g_free(prevs->filename);
      prevs->filename = g_strdup(pl_entry_filename);
    }
  }
}

static void
aosd_trigger_func_pb_pauseon_onoff ( gboolean turn_on )
{
  if ( turn_on == TRUE )
    hook_associate( "playback pause" , aosd_trigger_func_pb_pauseon_cb , NULL );
  else
    hook_dissociate( "playback pause" , aosd_trigger_func_pb_pauseon_cb );
  return;
}

static void
aosd_trigger_func_pb_pauseon_cb ( gpointer unused1 , gpointer unused2 )
{
#ifdef DEBUG
  printf("Paused\n");
#endif
  updatestate(VFDM_PAUSED);
  return;
}


static void
aosd_trigger_func_pb_pauseoff_onoff ( gboolean turn_on )
{
  if ( turn_on == TRUE )
    hook_associate( "playback unpause" , aosd_trigger_func_pb_pauseoff_cb , NULL );
  else
    hook_dissociate( "playback unpause" , aosd_trigger_func_pb_pauseoff_cb );
  return;
}

static void
aosd_trigger_func_pb_pauseoff_cb ( gpointer unused1 , gpointer unused2 )
{
#ifdef DEBUG
  printf("Unpaused\n");
#endif
  updatestate(VFDM_PLAYING);
  return;
}

static void
aosd_trigger_func_pb_stop_onoff ( gboolean turn_on )
{
  if ( turn_on == TRUE )
    hook_associate( "playback stop" , aosd_trigger_func_pb_stop_cb , NULL );
  else
    hook_dissociate( "playback stop" , aosd_trigger_func_pb_stop_cb );
  return;
}

static void
aosd_trigger_func_pb_stop_cb ( gpointer unused1 , gpointer unused2 )
{
#ifdef DEBUG
  printf("Stopped\n");
#endif
  updatestate(VFDM_STOPPED);
  return;
}
