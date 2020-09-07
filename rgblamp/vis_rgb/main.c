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
#include "soundbuf.h"

/* Do clean shutdown of sound and lamp connection on ^C */

static int rgbm_quit = 0;

static void sig_handler(int signo)
{
    if (signo == SIGINT) {
        rgbm_quit = 1;
    }
}

int rgbm_pollquit(void)
{
    return rgbm_quit;
}

int main(int argc, char **argv) {
    char *snddev = NULL;

    if (argc == 2) {
        snddev = argv[1];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [sound device]\n", argv[0]);
        return -1;
    }

    signal(SIGINT, sig_handler);

    rgbm_run(snddev);

    return 0;
}
