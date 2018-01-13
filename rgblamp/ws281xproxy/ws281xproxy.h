typedef double *pixel;

/* Common routines */

void fatal(const char *s);
pixel pix_alloc(void);

/* Driver routines for output */

#define COLORCNT 3
#ifndef PIXCNT
#define PIXCNT 60
#endif

void render_open(void);
void render_close(void);

void render(const pixel pixr);
void render_1pwm(const unsigned short *pix);
void render_1srgb(const pixel pix);
