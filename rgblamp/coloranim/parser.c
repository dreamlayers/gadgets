#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "coloranim.h"

const unsigned int components = 3;

const char *(*parse_getnext)(void) = NULL;
const char *(*parse_peeknext)(void) = NULL;
int (*parse_eof)(void) = NULL;
void (*parse_rewind)(void) = NULL;

static int parse_double(double *d)
{
    const char *s = parse_getnext();
    char *endptr;
    if (s == NULL) return -1;
    *d = strtod(s, &endptr);
    DEBUG_PRINT("D: %f\n", *d);
    return (*endptr == 0) ? 0 : -1;
}

static double parse_zerotoone(void)
{
    double d;
    int r;

    r = parse_double(&d);
    return (r == 0 && d >= 0 && d <= 1.0) ? d : -1.0;
}

static int parse_isnum(void)
{
    if (parse_eof()) {
        return 0;
    } else {
        const char *s = parse_peeknext();
        return (*s >= '0' && *s <= '9') || *s == '.';
    }
}

static int parse_rgb(pixel res)
{
    DEBUG_PRINT("RGB\n");
    if (parse_isnum()) {
        int i;
        res[0] = parse_zerotoone();
        if (res[0] < 0) return -1;
        if (parse_isnum()) {
            for (i = 1; i < components; i++) {
                res[i] = parse_zerotoone();
                if (res[i] < 0) return -1;
            }
        } else {
            for (i = 1; i < components; i++) {
                res[i] = res[0];
            }
        }
        return 0;
    } else {
        return -1; /* Color names not supported */
    }
}

typedef struct {
    char *name;
    keyword val;
} keywordmap;

static keyword parse_keyword(const keywordmap *keywords)
{
    const char *s = parse_peeknext();
    const keywordmap *tok = &keywords[0];
    if (s == NULL) return KW_ERROR;
    while (tok->name != NULL) {
        if (strcmp(tok->name, s) == 0) {
            (void)parse_getnext();
            return tok->val;
        }
        tok++;
    }
    return KW_NONE;
}

static keyword parse_color_keyword(void)
{
    const keywordmap kw_color[] = {
        { "to", KW_GRADIENT },
        { NULL, KW_NONE }
    };
    return parse_keyword(kw_color);
}

static keyword parse_transition_keyword(void)
{
    const keywordmap kw_transition[] = {
        { "in", KW_CROSSFADE },
        { "for", KW_FOR },
        { NULL, KW_NONE }
    };
    return parse_keyword(kw_transition);
}

static double parse_transarg(keyword kw) {
    double t = 0.0;
    switch (kw) {
    case KW_CROSSFADE:
        if (parse_double(&t) != 0) {
            return -1.0;
        }
        break;
    default:
        break;
    }
    return t;
}

static keyword parse_looping_keyword(void)
{
    const keywordmap kw_looping[] = {
        { "repeat", KW_REPEAT },
        { NULL, KW_NONE }
    };
    return parse_keyword(kw_looping);
}

int parse_main(void)
{
    keyword kw = EXPECT_COLOR;
    keyword trans = KW_NONE, newtrans;
    double transarg = 0.0;

    pixel colorspec = pix_alloc();
    unsigned int specidx = 0;
    keyword colorkw[60];

    pixel tempclr, oldclr = pix_alloc(), newclr = pix_alloc();

    enum parser_state {
        EXPECT_COLOR,
        AFTER_COLOR,
    };

    while (!cmd_cb_pollquit()) {
        keyword looping;

        DEBUG_PRINT("PARSE\n");
        if (parse_rgb(&colorspec[specidx * COLORCNT]) != 0) return -1;
        colorkw[specidx++] = KW_NONE;
        if (parse_eof()) {
            newtrans = KW_NONE;
        } else {
            kw = parse_color_keyword();
            if (kw == KW_ERROR) {
                return -1;
            } else if (kw != KW_NONE) {
                colorkw[specidx - 1] = kw;
                continue;
            }

            newtrans = parse_transition_keyword();
            DEBUG_PRINT("%i\n", newtrans);
            if (newtrans == KW_ERROR || newtrans == KW_NONE) return -1;
        }

        /* This actually does the previous transition, not the one just read */
        if (fx_makestate(colorspec, colorkw, specidx, newclr) != 0) return -1;
        if (fx_transition(oldclr, trans, transarg, newclr) != 0) return -1;

        if (parse_eof()) break;

        trans = newtrans;
        specidx = 0;

        tempclr  = oldclr;
        oldclr = newclr;
        newclr = tempclr;

        transarg = parse_transarg(trans);
        if (transarg < 0) return -1;

        looping = parse_looping_keyword();
        switch (looping) {
        case KW_REPEAT:
            parse_rewind();
            break;
        case KW_NONE:
            break;
        default:
            return -1;
        }
    }
    return 0;
}

#if 0
/* --- command line argument parser --- */

static const char **sargv;
static unsigned int argidx, sargc;

static const char *parse_args_getnext(void)
{
    if (argidx >= sargc) parse_fatal("getnext past end of argument list");
    return sargv[argidx++];
}

static const char *parse_args_peeknext(void)
{
    if (argidx >= sargc) parse_fatal("peeknext past end of argument list");
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

void parse_args(int argc, char **argv)
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
#endif /* 0 */
