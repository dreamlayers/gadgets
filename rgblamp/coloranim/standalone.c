#include <stdlib.h>
#include "coloranim.h"

/* --- command line argument parser --- */

static const char **sargv;
static unsigned int argidx, sargc;

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

static void parse_args(int argc, char **argv)
{
    sargc = argc;
    argidx = 1;
    sargv = (const char **)argv;
    parse_getnext = parse_args_getnext;
    parse_peeknext = parse_args_peeknext;
    parse_eof = parse_args_eof;
    parse_rewind = parse_args_rewind;
    parse_main();
}

/* TODO catch signals and quit in response */
int cmd_cb_pollquit(void)
{
    return 0;
}

/* Standalone command line coloranim */
int main(int argc, char **argv)
{
    render_open();
    parse_init();

    parse_args(argc, argv);

    parse_quit();
    render_close();

    return 0;
}
