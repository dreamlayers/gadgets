#ifdef __GNUC__
#define __cdecl
#endif

#include <kodi/xbmc_vis_types.h>
#include <kodi/xbmc_vis_dll.h>

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#define RGBM_AUDACIOUS
#include "../rgbm.h"

bool initialized = false;

bool warnGiven = false;

float audio_data[513];
float audio_data_freq[513];


extern "C" ADDON_STATUS ADDON_Create (void* hdl, void* props)
{
    if (!props)
        return ADDON_STATUS_UNKNOWN;

    //VIS_PROPS* visProps = (VIS_PROPS*) props;

    if (!initialized) {
        if (!rgbm_init())
            return ADDON_STATUS_UNKNOWN;
        initialized = true;
    }


    return ADDON_STATUS_NEED_SETTINGS;
}

extern "C" void Start (int, int, int, const char*)
{
}

extern "C" void AudioData (const float *pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
    if (initialized) {
        rgbm_render(pFreqData);
    }
#if 0
    if ((unsigned int)iAudioDataLength > sizeof(audio_data)/sizeof(*audio_data)-1) {
        if (!warnGiven) {
            fprintf(stderr, "Unexpected audio data length received (%d), expect incorrect vis\n", iAudioDataLength);
            warnGiven = true;
        }
        return;
    }

    if ((unsigned int)iFreqDataLength > sizeof(audio_data_freq)/sizeof(*audio_data_freq)-1) {
        if (!warnGiven) {
            fprintf(stderr, "Unexpected freq data length received (%d), expect incorrect vis\n", iAudioDataLength);
            warnGiven = true;
        }
        return;
    }

    for (unsigned long i=0; i<sizeof(audio_data)/sizeof(*audio_data); i++) { audio_data[i] = 0; }
    for (unsigned long i=0; i<sizeof(audio_data_freq)/sizeof(*audio_data_freq); i++) { audio_data_freq[i] = 0; }

    memcpy(audio_data, pAudioData, iAudioDataLength*sizeof(float));
    memcpy(audio_data_freq, pFreqData, iFreqDataLength*sizeof(float));
#endif
}

extern "C" void Render()
{
    /*for (unsigned long i = 0; i < 512; i++)
    {
        audio_data[i] = (float)(rand()%65535-32768)*(1.0f/32768.0f);
    }
    for (unsigned long i = 0; i < 512; i++)
    {
        audio_data_freq[i] = (float)(rand()%65535)*(1.0f/65535.0f);
    }*/
}

extern "C" void GetInfo (VIS_INFO* pInfo)
{
    pInfo->bWantsFreq = true;
    pInfo->iSyncDelay = 0;
}

extern "C" bool OnAction (long flags, const void *param)
{
    return false;
}

extern "C" unsigned int GetPresets (char ***presets)
{
    return 0;
}

extern "C" unsigned GetPreset()
{
    return 0;
}

extern "C" bool IsLocked()
{
    return false;
}

extern "C" unsigned int GetSubModules (char ***names)
{
    return 0;
}

extern "C" void ADDON_Stop()
{
    return;
}

extern "C" void ADDON_Destroy()
{
    if (initialized) {
        rgbm_shutdown();
        initialized = false;
    }
    return;
}

extern "C" bool ADDON_HasSettings()
{
    return false;
}

extern "C" ADDON_STATUS ADDON_GetStatus()
{
    //    return ADDON_STATUS_UNKNOWN;

    return ADDON_STATUS_OK;
}

extern "C" unsigned int ADDON_GetSettings (ADDON_StructSetting ***sSet)
{
    return 0;
}

extern "C" void ADDON_FreeSettings()
{
}

extern "C" ADDON_STATUS ADDON_SetSetting (const char *strSetting, const void* value)
{
    return ADDON_STATUS_OK;
}

extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}
