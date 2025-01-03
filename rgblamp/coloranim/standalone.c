#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include <string.h>
#include <signal.h>
#endif
#include "coloranim.h"

/* --- command line argument parser --- */

static const char **sargv;
static unsigned int argidx, sargc;

#ifndef WIN32
static int quit = 0;

static void sig_quit(int sig) {
    quit = 1;
}
#endif /* !WIN32 */

static const char *parse_args_getnext(void)
{
    if (argidx >= sargc) return NULL; /* past end of argument list */
    return sargv[argidx++];
}

static const char *parse_args_peeknext(void)
{
    if (argidx >= sargc) return NULL; /* past end of argument list */
    return sargv[argidx];
}

static int parse_args_eof(void)
{
    return argidx >= sargc;
}

static void parse_args_rewind(void)
{
    argidx = 1;
}

static int parse_args(int argc, char **argv)
{
    sargc = argc;
    argidx = 1;
    sargv = (const char **)argv;
    parse_getnext = parse_args_getnext;
    parse_peeknext = parse_args_peeknext;
    parse_eof = parse_args_eof;
    parse_rewind = parse_args_rewind;

    return parse_and_run();
}

int cmd_cb_pollquit(void)
{
#ifdef WIN32
    return 0;
#else
    return quit;
#endif
}

void coloranim_notify(void)
{
}

/* Standalone command line coloranim */
int main(int argc, char **argv)
{
    struct sigaction sa;

    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = sig_quit;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    render_open();
    coloranim_init();
    parse_init();

    if (parse_args(argc, argv) != 0) {
        fprintf(stderr, "Parse error!\n");
    }

    parse_quit();
    coloranim_quit();
    render_close();

    return 0;
}
