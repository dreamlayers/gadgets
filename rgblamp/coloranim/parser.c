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

static pixel colorspec = NULL, oldclr = NULL, newclr = NULL;

void parse_init(void)
{
    colorspec = pix_alloc();
    oldclr = pix_alloc();
    newclr = pix_alloc();
}

void parse_quit(void)
{
    pix_free(&colorspec);
    pix_free(&oldclr);
    pix_free(&newclr);
}

int parse_main(void)
{
    keyword trans = KW_NONE;
    double transarg = 0.0;

    unsigned int specidx = 0;
    keyword colorkw[60];
    pixel tempclr;

    while (!cmd_cb_pollquit()) {
        keyword kw;

        DEBUG_PRINT("PARSE\n");
        if (parse_rgb(&colorspec[specidx * COLORCNT]) != 0) return -1;
        colorkw[specidx++] = KW_NONE;
        if (!parse_eof()) {
            kw = parse_color_keyword();
            if (kw == KW_ERROR) {
                return -1;
            } else if (kw != KW_NONE) {
                colorkw[specidx - 1] = kw;
                continue;
            }
        }

        if (fx_makestate(colorspec, colorkw, specidx, newclr) != 0) return -1;
        specidx = 0;

        if (fx_transition(oldclr, trans, transarg, newclr) != 0) return -1;
        tempclr  = oldclr;
        oldclr = newclr;
        newclr = tempclr;

        if (parse_eof()) break;

        trans = parse_transition_keyword();
        DEBUG_PRINT("%i\n", trans);
        if (trans == KW_ERROR || trans == KW_NONE) return -1;

        transarg = parse_transarg(trans);
        if (transarg < 0) return -1;

        kw = parse_looping_keyword();
        switch (kw) {
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
