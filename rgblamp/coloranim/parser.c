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

static double parse_brightness(void)
{
    double d;
    int r;
    const char *s = parse_peeknext();

    if (s[0] == '-'  && s[1] == 0) {
        (void)parse_getnext();
        return -1.0;
    } else {
        r = parse_double(&d);
        return (r == 0 && d >= 0 && d <= 1.0) ? d : -2.0;
    }
}

static int parse_isbrightness(void)
{
    if (parse_eof()) {
        return 0;
    } else {
        const char *s = parse_peeknext();
        return (*s >= '0' && *s <= '9') || *s == '.' || *s == '-';
    }
}

static int parse_rgb(pixel res)
{
    DEBUG_PRINT("RGB\n");
    if (parse_isbrightness()) {
        int i;
        res[0] = parse_brightness();
        if (res[0] < -1.5) return -1;
        if (parse_isbrightness()) {
            for (i = 1; i < components; i++) {
                res[i] = parse_brightness();
                if (res[i] < -1.5) return -1;
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
        { ":", KW_SOLID },
        { NULL, KW_NONE }
    };
    return parse_keyword(kw_color);
}

static int get_kw_if_match(const char *name)
{
    const char *s = parse_peeknext();
    if (s == NULL) {
        return -1; /* Input error */
    } else if (strcmp(name, s) == 0 && parse_getnext() != NULL) {
        return 1;  /* Keyword found and removed */
    } else {
        return 0;  /* No error, but keyword not found */
    }
}

static double parse_kw_posdbl(const char *name)
{
    int match = get_kw_if_match(name);

    if (match > 0) {
        double d;
        if (parse_double(&d) == 0) {
            return d;
        } else {
            return -1.0;
        }
    } else if (match == 0) {
        return 0.0;
    } else {
        return -1.0;
    }
}

static pixel colorspec = NULL;

void parse_init(void)
{
    colorspec = pix_alloc();
}

void parse_quit(void)
{
    pix_free(&colorspec);
}

void coloranim_free(struct coloranim **p)
{
    if (*p != NULL) {
        struct coloranim_state *state, *next_state;
        for (state = (*p)->states; state != NULL; state = next_state) {
            next_state = state->next;
            free(state);
        }
        free(*p);
        *p = NULL;
    }
}

struct coloranim *coloranim_parse(void)
{
    struct coloranim *ca;
    struct coloranim_state *cur_state, **next_p;

    unsigned int specidx = 0;
    keyword colorkw[PIXCNT];

    double fade_time = 0.0;

    pixel start_pix = pix_alloc(), prev_pix = start_pix;
    render_get(start_pix);

    ca = safe_malloc(sizeof(*ca));
    ca->repeat = 0;
    ca->states = NULL;
    next_p = &(ca->states);

    if (parse_eof()) goto parse_fail;
    ca->first_fade = parse_kw_posdbl("in");
    if (ca->first_fade < 0.0) goto parse_fail;

    while (1) {
        keyword kw;
        int repeat;

        DEBUG_PRINT("PARSE\n");
        if (specidx >= PIXCNT) goto parse_fail;
        if (parse_rgb(&colorspec[specidx * COLORCNT]) != 0) {
            goto parse_fail;
        }
        colorkw[specidx++] = KW_NONE;
        if (!parse_eof()) {
            kw = parse_color_keyword();
            if (kw == KW_ERROR) {
                goto parse_fail;
            } else if (kw != KW_NONE) {
                colorkw[specidx - 1] = kw;
                continue;
            }
        }

        cur_state = safe_malloc(sizeof(*cur_state));
        *next_p = cur_state;
        next_p = &(cur_state->next);

        cur_state->fade_to = fade_time;
        fade_time = 0.0;

        cur_state->pix = pix_alloc();
        if (fx_makestate(colorspec, colorkw, specidx,
                         prev_pix, cur_state->pix) != 0) goto parse_fail;
        prev_pix = cur_state->pix;
        specidx = 0;

        if (parse_eof()) {
            cur_state->hold_for = 0;
            break;
        }
        cur_state->hold_for = parse_kw_posdbl("for");
        if (cur_state->hold_for < 0.0) goto parse_fail;

        if (parse_eof()) break;
        fade_time = parse_kw_posdbl("in");
        if (fade_time < 0.0) goto parse_fail;

        if (parse_eof()) break;
        repeat = get_kw_if_match("repeat");
        if (repeat < 0) {
            goto parse_fail;
        } else if (repeat > 0) {
            if (!parse_eof() ||
                (cur_state->hold_for == 0.0 && fade_time == 0)) {
                goto parse_fail;
            } else {
                ca->repeat = 1;
                break;
            }
        }
    }

    *next_p = NULL;
    if (ca->repeat) {
        ca->states->fade_to = fade_time;
    } else if (fade_time != 0.0) {
        goto parse_fail;
    }
    pix_free(&start_pix);
    return ca;

parse_fail:
    *next_p = NULL;
    coloranim_free(&ca);
    pix_free(&start_pix);
    return NULL;
}
