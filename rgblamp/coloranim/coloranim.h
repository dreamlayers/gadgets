typedef double *pixel;

/* Common routines */

void fatal(const char *s);
pixel pix_alloc(void);

/* Driver routines for output */

#define COLORCNT 3
#ifndef PIXCNT
#define PIXCNT 1
#endif

void render_open(void);
void render(const pixel pixr);
void render_close(void);

/* Parser routines */

typedef enum {
    KW_NONE = 0,
    EXPECT_COLOR,
    KW_GRADIENT,
    KW_CROSSFADE,
    KW_FOR
} keyword;

void parse_fatal(const char *s);
void parse_args(int argc, char **argv);

/* Effect routines */

void fx_makestate(const pixel colorspec, const keyword *colorkw,
                  unsigned int numspec,
                  pixel dest);
void fx_transition(const pixel oldclr, keyword kw, double arg,
                   const pixel newclr);
