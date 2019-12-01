#include <stdio.h>
#include <string.h>
#include "jsmn.h"
#include "coloranim.h"

struct json_mapping {
    const char * const name;
    const int namelen;
    const jsmntype_t type;
    const char *data;
    int datalen;
    struct json_mapping * const map;
};

static struct json_mapping json_color[] = {
    { "r", 1, JSMN_PRIMITIVE, NULL, 0, NULL},
    { "g", 1, JSMN_PRIMITIVE, NULL, 0, NULL},
    { "b", 1, JSMN_PRIMITIVE, NULL, 0, NULL},
    { NULL, 0, JSMN_STRING, NULL, 0, NULL }
};

static int get_unsigned_int(const struct json_mapping *m) {
    const char *p, *e;
    unsigned int n = 0;

    if (m->data == NULL) return -1;
    for (p = m->data, e = p + m->datalen; p < e; p++) {
        char c = *p - '0';
        if (c < 0 || c > 9) return -1;
        n = n * 10 + c;
    }

    return n;
}

void get_color(int *rgb)
{
    int i, t;
    for (i = 0; i < 3; i++) {
        t = get_unsigned_int(&json_color[i]);
        if (t >= 0) rgb[i] = t;
    }
}

static struct json_mapping json_toplevel[] = {
    { "state", 5, JSMN_STRING, NULL, 0, NULL},
    { "brightness", 10, JSMN_PRIMITIVE, NULL, 0, NULL},
    { "color", 5, JSMN_OBJECT, NULL, 0, json_color},
    { "effect", 6, JSMN_STRING, NULL, 0, NULL},
    { NULL, 0, JSMN_STRING, NULL, 0, NULL }
};

void get_state(int *state)
{
    if (json_toplevel[0].datalen == 3 &&
        !memcmp(json_toplevel[0].data, "OFF", 3)) {
        *state = 0;
    } else if (json_toplevel[0].datalen == 2 &&
               !memcmp(json_toplevel[0].data, "ON", 2)) {
        *state = 1;
    }
}

void get_brightness(int *brightness)
{
    int newbright = get_unsigned_int(&json_toplevel[1]);
    if (newbright >= 0) *brightness = newbright;
}

void get_effect(char *effect, int l)
{
    if (json_toplevel[3].data == NULL
        || json_toplevel[3].datalen > (l - 1)) {
        effect[0] = 0;
    } else {
        memcpy(effect, json_toplevel[3].data,
               json_toplevel[3].datalen);
        effect[json_toplevel[3].datalen] = 0;
    }
}

static void json_mapping_clear(struct json_mapping *m) {
    for (; m->name != NULL; m++) {
        m->data = NULL;
        m->datalen = 0;
        if (m->map != NULL) {
            json_mapping_clear(m->map);
        }
    }
}

static int json_object_parse(const char *msg, const jsmntok_t *tok,
                             struct json_mapping *mapping)
{
    /* TODO: Add checks for not running off the end of tokens */
    int i, idx = 1;
    for (i = 0; i < tok[0].size; i++) {
        struct json_mapping *m;
        int l;

        if (tok[idx].type != JSMN_STRING) return -1;

        l = tok[idx].end - tok[idx].start;
        m = mapping;
        while (1) {
            if (m->name == NULL) return -1;
            if (l == m->namelen && tok[idx+1].type == m->type &&
                !memcmp(m->name, &msg[tok[idx].start], l)) {
                m->data = &msg[tok[idx+1].start];
                m->datalen = tok[idx+1].end - tok[idx+1].start;
                if (m->type == JSMN_OBJECT) {
                    json_object_parse(msg, &tok[idx+1], m->map);
                }
                break;
            }
            m++;
        }
        idx++;
        idx += 1 + tok[idx].size;
    }
    return 0;
}

int json_parse(const char *msg, int len)
{
    int r;
    jsmn_parser p;
    jsmntok_t tok[100];

    json_mapping_clear(json_toplevel);

    /* Prepare parser */
    jsmn_init(&p);

    r = jsmn_parse(&p, msg, len, tok, sizeof(tok)/sizeof(tok[0]));
    if (r >= 0 && tok[0].type == JSMN_OBJECT) {
        if (json_object_parse(msg, &tok[0], json_toplevel) >= 0) {
            return 1;
        }
    }
    return -1;
}
