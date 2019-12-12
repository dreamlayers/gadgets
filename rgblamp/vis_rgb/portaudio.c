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

/*
 * Visualizer waits until this many new samples have been collected.
 * Of course if it's running slow many more new may have arrived.
 * Visualizer will always get the latest RGBM_NUMSAMP samples.
 */
#define MIN_NEW_SAMPLES (RGBM_NUMSAMP * 2 / 3)

/* Default sound device, for visualizing playback from other programs */
#ifndef MONITOR_NAME
#ifdef WIN32
#define MONITOR_NAME "Stereo Mix"
#else
#define MONITOR_NAME "pulse_monitor"
#endif
#endif

static PaStream *stream = NULL;
static double *samp = NULL;

/*
 * Incoming samples go into a ring buffer protected by a mutex.
 * Next incoming sample goes to ring[ring_write], and ring_write wraps.
 * When enough samples have arrived sound_retrieve() copies from the ring
 * buffer, protected by that mutex, which prevents new samples from
 * overwriting. It uses ring_write to know how data is wrapped in the
 * ring buffer.
 */
static double ring[RGBM_NUMSAMP];
static unsigned int ring_write = 0;
static pthread_mutex_t mutex;
static pthread_cond_t cond;
static int signalled = 0;

/* Number of samples put into ring buffer since last sound_retrieve() call is
 * counted by ring_has. If visualizer is slower than input, ring_has could be
 * more than RGBM_NUMSAMP, which is useful for timing.
 */
static unsigned int ring_has = 0;

static void error(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
    exit(-1);
}

static void sound_store(const int16_t *input, unsigned int len) {
    int i;
    const int16_t *ip;
    double *op;
    unsigned int remain;

    if (len >= RGBM_NUMSAMP) {
        /* If there are too many samples, fill buffer with latest ones */
        ip = input + len - RGBM_NUMSAMP;
        op = &(ring[0]);
        remain = RGBM_NUMSAMP;

        ring_write = 0;
    } else {
        remain = RGBM_NUMSAMP - ring_write;
        if (remain >= len) {
            remain = len;
        } else {
            /* Do part of store after wrapping around ring */
            int remain2 = len - remain;
            ip = input + remain;
            op = &(ring[0]);
            for (i = 0; i < remain2; i++) {
                *(op++) = *(ip++) / 32768.0;
            }
        }

        ip = input;
        op = &(ring[ring_write]);

        /* Advance write pointer */
        ring_write += len;
        if (ring_write >= RGBM_NUMSAMP) {
            ring_write = 0;
        }
    }

    /* Do part of store before wrapping point, maybe whole store */
    for (i = 0; i < remain; i++) {
        *(op++) = *(ip++) / 32768.0;
    }

    /* This could theoretically overflow, but is needed for timing */
    ring_has += len;
}

static int pa_callback(const void *input, void *output,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags, void *userData) {
    pthread_mutex_lock(&mutex);
    sound_store((int16_t *)input, frameCount);
    if (ring_has >= MIN_NEW_SAMPLES && !signalled) {
        /* Next buffer is full */
        signalled = 1;
        pthread_cond_signal(&cond);
    }
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
                      0,
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
    unsigned int chunk1, ring_had;

    pthread_mutex_lock(&mutex);

    /* Wait for next buffer to be full */
    while (!signalled) {
        pthread_cond_wait(&cond, &mutex);
    }
    signalled = 0;

    /* Copy first part, up to wrapping point */
    chunk1 = RGBM_NUMSAMP - ring_write;
    memcpy(&samp[0], &ring[ring_write], sizeof(double) * chunk1);
    if (ring_write > 0) {
        /* Samples are wrapped around the ring. Copy the second part. */
        memcpy(&samp[chunk1], &ring[0], sizeof(double) * ring_write);
    }

    /* Unnecessary but optimal if visualization is faster than input. */
    ring_write = 0;

    ring_had = ring_has;
    ring_has = 0;

    /* After swap the buffer shouldn't be touched anymore by interrupts */
    pthread_mutex_unlock(&mutex);

    return ring_had;
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
