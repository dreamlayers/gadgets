#ifdef __GNUC__
#define __cdecl
#endif

#include <kodi/addon-instance/Visualization.h>

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

class CRGBLampVisualization : public kodi::addon::CAddonBase,
                         public kodi::addon::CInstanceVisualization
{
public:
  CRGBLampVisualization();
  bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  void Stop(void) override;
  void AudioData(const float* pAudioData, int iAudioDataLength,
                 float* pFreqData, int iFreqDataLength) override;
  void Render() override;
  void GetInfo (bool &wantsFreq, int &syncDelay) override;
};

CRGBLampVisualization::CRGBLampVisualization()
{
    if (!initialized && rgbm_init())
        initialized = true;
}

bool CRGBLampVisualization::Start(int channels, int samplesPerSec,
                             int bitsPerSample, std::string songName)
{
    return initialized;
}

void CRGBLampVisualization::AudioData(const float* pAudioData, int iAudioDataLength,                                 float* pFreqData, int iFreqDataLength)
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

void CRGBLampVisualization::Render()
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

void CRGBLampVisualization::GetInfo (bool &wantsFreq, int &syncDelay)
{
    wantsFreq = true;
    syncDelay = 0;
}

void CRGBLampVisualization::Stop(void)
{
    if (initialized) {
        rgbm_shutdown();
        initialized = false;
    }
}

ADDONCREATOR(CRGBLampVisualization);
