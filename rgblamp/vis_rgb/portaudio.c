/* Standalone sound code using PortAudio for RGB lamp visualizer. */
/* Copyright 2017 Boris Gjenero. Released under the MIT license. */


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <portaudio.h>
#include <pthread.h>
#include "rgbm.h"

#define inputsize (RGBM_NUMSAMP * 2 / 3)

/* Default sound device, for visualizing playback from other programs */
#ifdef WIN32
#define MONITOR_NAME "Stereo Mix"
#else
#define MONITOR_NAME "pulse_monitor"
#endif

static PaStream *stream = NULL;
static double *samp = NULL;
/* Each ring contains enough samples for one output block. Input block
 * size must be smaller or equal to output block size. Input samples
 * are added at ring_write, wrapping around the rings. They are copied
 * from there to the output buffers, forming un-wrapped output blocks.
 */
static double ring[RGBM_NUMSAMP];
static int ring_write = 0;
static pthread_mutex_t mutex;
static pthread_cond_t cond;
static unsigned long frames_available = 0;

static void error(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
    exit(-1);
}

static void sound_store(const int16_t *input) {
    int i, outidx;

    outidx = ring_write;
    for (i = 0; i < inputsize; i++) {
        ring[outidx] = input[i] / 32768.0;
        outidx++;
        if (outidx >= RGBM_NUMSAMP) outidx = 0;
    }
    ring_write = outidx;
}

static int pa_callback(const void *input, void *output,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags, void *userData) {
    if (frameCount != inputsize) {
        error("callback got unexpected number of samples.");
    }

    pthread_mutex_lock(&mutex);
    sound_store((int16_t *)input);
    frames_available += frameCount;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    return paContinue;
}

static void sound_open(const char *devname) {
    PaError err;
    PaStreamParameters inputParameters;
    PaDeviceIndex numDevices;
#ifdef WIN32
    unsigned int namelen;
#endif

    err = Pa_Initialize();
    if(err != paNoError) error("initializing PortAudio.");

    if ((devname == NULL) ?
#ifdef WIN32
        0
#else
        /* PULSE_SOURCE sets default input device. Don't automatically
           over-ride that with our monitor name guess. */
        (getenv("PULSE_SOURCE") != NULL)
#endif
        /* Use "default" to explicitly select default sound source.
           Needed because otherwise MONITOR_NAME would be used. */
        : (!strcmp(devname, "default"))) {
        inputParameters.device = Pa_GetDefaultInputDevice();
    } else {
        if (devname == NULL) devname = MONITOR_NAME;

        /* Search through devices to find one matching devname */
        numDevices = Pa_GetDeviceCount();
#ifdef WIN32
        namelen = strlen(devname);
#endif
        for (inputParameters.device = 0; inputParameters.device < numDevices;
             inputParameters.device++) {
            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
            if (deviceInfo != NULL && deviceInfo->name != NULL &&
#ifdef WIN32
                /* Allow substring matches, for eg. "Stereo Mix (Sound card name)" */
                !strncmp(deviceInfo->name, devname, namelen)
#else
                !strcmp(deviceInfo->name, devname)
#endif
                ) break;
        }
        if (inputParameters.device == numDevices) {
            fprintf(stderr, "Warning: couldn't find %s sound device, using default\n",
                    devname);
            inputParameters.device = Pa_GetDefaultInputDevice();
        }
    }

    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16; /* PortAudio uses CPU endianness. */
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(&stream,
                      &inputParameters,
                      NULL, //&outputParameters,
                      44100,
                      inputsize,
                      paClipOff,
                      pa_callback,
                      NULL); /* no callback userData */
    if(err != paNoError) error("opening PortAudio stream");

    err = Pa_StartStream( stream );
    if(err != paNoError) error("starting PortAudio stream");

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

static void sound_close(void) {
    PaError err;

    err = Pa_StopStream(stream);
    if (err != paNoError) error("stopping PortAudio stream");

    err = Pa_CloseStream(stream);
    if (err != paNoError) error("closing PortAudio stream");

    err = Pa_Terminate();
    if (err != paNoError) error("terminating PortAudio");

    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
}

static unsigned long sound_retrieve(void) {
    unsigned long res;

    pthread_mutex_lock(&mutex);

    while (frames_available == 0) {
        pthread_cond_wait(&cond, &mutex);
    }

    memcpy(&samp[0], &ring[ring_write],
           sizeof(double) * (RGBM_NUMSAMP - ring_write));
    if (ring_write > 0) {
        /* Samples are wrapped around the ring. Copy the second part. */
        memcpy(&samp[RGBM_NUMSAMP - ring_write], &ring[0],
               sizeof(double) * ring_write);
    }

    res = frames_available;
    frames_available = 0;

    pthread_mutex_unlock(&mutex);

    return res;
}


/* Do clean shutdown of sound and lamp connection on ^C */
static int quit;
static void sig_handler(int signo)
{
    if (signo == SIGINT) {
        quit = 1;
    }
}

static void sound_visualize(void) {
    double deltat;
    do {
        if (quit) break;
        deltat = (double)sound_retrieve() / 44100.0;
    } while (rgbm_render_wave(deltat));
}

int main(int argc, char **argv) {
    char *snddev = NULL;
    quit = 0;

    if (argc == 2) {
        snddev = argv[1];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [sound device]\n", argv[0]);
        return -1;
    }

    signal(SIGINT, sig_handler);

    if (!rgbm_init()) {
        error("initializing visualization");
    }
    samp = rgbm_get_wave_buffer();

    sound_open(snddev);

    sound_visualize();

    sound_close();
    rgbm_shutdown();

    return 0;
}
