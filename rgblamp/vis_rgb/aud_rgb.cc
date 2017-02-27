/* Audacious-specific visualization plugin code for the RGB lamp. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <glib.h>
#include <libaudcore/i18n.h>
#define AUD_PLUGIN_GLIB_ONLY
#include <libaudcore/plugin.h>
#include "rgbm.h"

class ARGB : public VisPlugin
{
public:
    static const char about[];

    static constexpr PluginInfo info = {
        N_("Audacious RGB"),
        N_("gadgets"),
        about
    };

    constexpr ARGB () : VisPlugin (info, Visualizer::Freq) {}

    bool init ();
    void cleanup ();

    /* intensity of frequencies 1/512, 2/512, ..., 256/512 of sample rate */
    void render_freq (const float * freq);
    void clear ();
};


__attribute__((visibility("default"))) ARGB aud_plugin_instance;

gboolean plugin_is_active = FALSE;

const char ARGB::about[] =
 N_("Audacious RGB\n");

bool ARGB::init(void)
{
    int i = rgbm_init();
    return i;
}

void ARGB::cleanup(void)
{
    rgbm_shutdown();
}

void ARGB::render_freq(const gfloat *freq)
{
    rgbm_render(freq);
}

void ARGB::clear()
{
    // Not sure if ignoring this is the best strategy
}
