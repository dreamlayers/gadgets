#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "coloranim.h"

const unsigned int components = 3;

static const char *(*parse_getnext)(void) = NULL;
static const char *(*parse_peeknext)(void) = NULL;
static int (*parse_eof)(void) = NULL;
static void (*parse_rewind)(void) = NULL;

void parse_fatal(const char *s) __attribute__ ((noreturn));
void parse_fatal(const char *s)
{
    fprintf(stderr, "Fatal parse error: %s\n", s);
    exit(-1);
}

static double parse_double(void)
{
    double d;
    const char *s = parse_getnext();
    char *endptr;
    d = strtod(s, &endptr);
    printf("D: %f\n", d);
    if (*endptr == 0) {
        return d;
    } else {
        parse_fatal("bad floating point value");
    }
}

static double parse_zerotoone(void)
{
    double d = parse_double();
    if (d >= 0 && d <= 1.0) {
        return d;
    } else {
        parse_fatal("value should be between 0 and 1 inclusive");
    }
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

static pixel parse_colorname(void)
{
    parse_fatal("color names not implemented");
}

static pixel parse_rgb(pixel res)
{
    printf("RGB\n");
    if (parse_isnum()) {
        int i;
        res[0] = parse_zerotoone();
        if (parse_isnum()) {
            for (i = 1; i < components; i++) {
                res[i] = parse_zerotoone();
            }
        } else {
            for (i = 1; i < components; i++) {
                res[i] = res[0];
            }
        }
        return res;
    } else {
        return parse_colorname();
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
    switch (kw) {
    case KW_CROSSFADE:
        return parse_double();
    default:
        return 0.0;
        break;
    }
}

static keyword parse_looping_keyword(void)
{
    const keywordmap kw_looping[] = {
        { "repeat", KW_REPEAT },
        { NULL, KW_NONE }
    };
    return parse_keyword(kw_looping);
}

static void parse_main(void)
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

    while (!parse_eof()) {
        keyword looping;

        printf("PARSE\n");
        parse_rgb(&colorspec[specidx * COLORCNT]);
        colorkw[specidx++] = KW_NONE;
        if (parse_eof()) {
            newtrans = KW_NONE;
        } else {
            kw = parse_color_keyword();
            if (kw != KW_NONE) {
                colorkw[specidx - 1] = kw;
                continue;
            }

            newtrans = parse_transition_keyword();
            printf("%i\n", newtrans);
            if (newtrans == KW_NONE) parse_fatal("unknown keyword");
        }

        /* This actually does the previous transition, not the one just read */
        fx_makestate(colorspec, colorkw, specidx, newclr);
        fx_transition(oldclr, trans, transarg, newclr);

        trans = newtrans;
        specidx = 0;

        tempclr  = oldclr;
        oldclr = newclr;
        newclr = tempclr;

        transarg = parse_transarg(trans);

        looping = parse_looping_keyword();
        switch (looping) {
        case KW_REPEAT:
            parse_rewind();
            break;
        case KW_NONE:
            break;
        default:
            parse_fatal("unknown looping keyword");
        }
    }
}

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
