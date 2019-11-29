#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

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

/* Passing data to the parser */

const char *(*parse_getnext)(void);
const char *(*parse_peeknext)(void);
int (*parse_eof)(void);
void (*parse_rewind)(void);

/* Parser routines */

typedef enum {
    KW_ERROR = -1,
    KW_NONE = 0,
    EXPECT_COLOR,
    KW_GRADIENT,
    KW_CROSSFADE,
    KW_FOR,
    KW_REPEAT
} keyword;

void parse_args(int argc, char **argv);
int parse_main(void);

/* Effect routines */

int fx_makestate(const pixel colorspec, const keyword *colorkw,
                 unsigned int numspec,
                 pixel dest);
int fx_transition(const pixel oldclr, keyword kw, double arg,
                  const pixel newclr);

/* MQTT interface routines */

int mqtt_init(void);
void mqtt_quit(void);

int json_parse(const char *msg, int len);
void mqtt_new_state(void);
void get_state(int *state);
void get_brightness(int *brightness);
void get_color(int *rgb);
