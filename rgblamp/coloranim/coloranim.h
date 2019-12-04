/* FIXME parts of signd.h are here so the socket stuff doesn't need to be
 * included when only these parts are needed
 */

void cmd_enq_string(int cmd, char *data, unsigned int len);
int cmd_cb_pollquit(void);

/* For debugging */

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

typedef double *pixel;

/* Common routines */

void fatal(const char *s);
void *safe_malloc(unsigned int l);
pixel pix_alloc(void);
void pix_clear(pixel pix);

void pix_free(pixel *p);

/* Driver routines for output */

#define COLORCNT 3
#ifndef PIXCNT
#define PIXCNT 1
#endif

void render_open(void);
void render(const pixel pixr);
void render_get(pixel pix);
void render_close(void);

/* Data structure for holding results of parsing */

struct coloranim_state {
    double fade_to;  /* Time in seconds for fading to this state */
    double hold_for; /* Time in seconds for staying at this state */
    pixel pix;
    struct coloranim_state *next;
};

struct coloranim {
    int repeat;
    double first_fade;
    struct coloranim_state *states;
};

/* Passing data to the parser */

const char *(*parse_getnext)(void);
const char *(*parse_peeknext)(void);
int (*parse_eof)(void);
void (*parse_rewind)(void);

/* Parser routines */

typedef enum {
    KW_ERROR = -1,
    KW_NONE = 0,
    KW_GRADIENT,
    KW_SOLID,
} keyword;

void parse_init(void);
void parse_quit(void);
void coloranim_free(struct coloranim **p);
struct coloranim *coloranim_parse(void);

/* Used by parser to create state */
int fx_makestate(const pixel colorspec, const keyword *colorkw,
                 unsigned int numspec,
                 pixel dest);

void coloranim_init(void);
void coloranim_quit(void);
void coloranim_exec(struct coloranim *ca);

int parse_and_run(void);

/* MQTT interface routines */

int mqtt_init(void);
void mqtt_quit(void);

int json_parse(const char *msg, int len);
void get_state(int *state);
void get_brightness(int *brightness);
void get_color(int *rgb);
void get_effect(char *effect, int len);

/* Effect routines */

int effect_list_len(void);
void effect_list_fill(char *p);
const char *preset_get(const char *name);
