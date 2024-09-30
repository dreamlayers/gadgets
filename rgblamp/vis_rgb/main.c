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

/* Hack to allow rgbm_multi to receive arguments. TODO: Properly implement it */
#ifndef DEFAULT_DRIVER
#define DEFAULT_DRIVER "lamp"
#endif
char *light_drivers = DEFAULT_DRIVER;

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

static void usage(void) {
    int i;
    printf("USAGE: rgbvis [light drivers] [sound device]\n\n"
           "VISUALIZATION PARAMETERS:\n"
           "NAME           DEFAULT  DESCRPTION\n");
    for (i = 0; rgbm_params[i].name != NULL; i++) {
        printf("%-9s %12.3f  %s\n", rgbm_params[i].name, rgbm_params[i].def,
               rgbm_params[i].description);
    }
    printf("\nSpecify visualization parameter as -name=value"
           " anywhere on command line."
           "\nValues prefixed with + or - add or subtract from default."
           "\nValues suffixed with %% refer to percentage of default.\n");
    exit(1);
}

static double parse_paramval(const char *s, double curval)
{
    double val;
    const char *p;
    char *endptr;
    if (s[0] == '-' || s[0] == '+') {
        p = &s[1];
    } else {
        p = s;
    }
    val = strtod(p, &endptr);
    if (endptr == p) return -1.0;

    if (*endptr == '%' && *(endptr+1) == 0) {
        val = curval * val / 100.0;
    } else if (*endptr != 0) {
        return -1.0;
    }

    if (s[0] == '-') {
        return curval - val;
    } else if (s[0] == '+') {
        return curval + val;
    } else {
        return val;
    }
}

static void parse_param(const char *s)
{
    int i, l;
    const char *end;

    end = strchr(s, '=');
    if (end == NULL) {
        fprintf(stderr, "Value not found in -%s parameter.\n\n", s);
        usage();
    }
    l = end - s;

    for (i = 0; rgbm_params[i].name != NULL; i++) {
        if (!strncmp(rgbm_params[i].name, s, l) &&
            rgbm_params[i].name[l] == 0) {
            double val = parse_paramval(end + 1, rgbm_params[i].def);
            if (val < 0) {
                fprintf(stderr, "Error in -%s numeric parameter.\n\n", s);
                usage();
            }
            *rgbm_params[i].value = val;
            printf("Parameter %s changed from %f to %f.\n", rgbm_params[i].name,
                   rgbm_params[i].def, val);
            return;
        }
    }
    fprintf(stderr, "Parameter name in -%s not found.\n\n", s);
    usage();
}

int main(int argc, char **argv) {
    char *snddev = NULL;
    char **posarg[] = { &light_drivers, &snddev, NULL };
    char ***argidx = &posarg[0];
    int i;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            usage();
        } else if (argv[i][0] == '-') {
            parse_param(&argv[i][1]);
        } else {
            if (*argidx != NULL) {
                **argidx = argv[i];
                argidx++;
            } else {
                fprintf(stderr, "Too many positional arguments supplied.\n\n");
                usage();
            }
        }
    }

    signal(SIGINT, sig_handler);

    rgbm_run(snddev);

    return 0;
}
