#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "coloranim.h"

struct preset {
    const char *name;
    const char *data;
};

const struct preset builtin_presets[] = {
    { "HSV_hue_rotation", "1 0 0 in 5 1 1 0 in 5 0 1 0 in 5 0 1 1 in 5 0 0 1 in 5 1 0 1 in 5 repeat" },
#if PIXCNT > 1
    { "red_and_green_sides_exchange", "1 0 0 to 0 1 0 in 10 0 1 0 to 1 0 0 in 10 repeat" },
#endif
    { "pulsate", "0.5 in 2 1 in 2 repeat" },
    { "red_green", "1 0 0 in 2 0 1 0 in 2 repeat" },
    { NULL, NULL }
};

int effect_list_len(void) {
    int n, l = 0;
    for (n = 0; builtin_presets[n].name != NULL; n++) {
        l += strlen(builtin_presets[n].name);
    }

    return 2 + 4 * n + l - 1 + 1;
}

void effect_list_fill(char *p) {
    int i, l;

    *(p++) = '[';
    *(p++) = ' ';

    for (i = 0; builtin_presets[i+1].name != NULL; i++) {
        l = strlen(builtin_presets[i].name);
        *(p++) = '"';
        memcpy(p, builtin_presets[i].name, l);
        p += l;
        *(p++) = '"';
        *(p++) = ',';
        *(p++) = ' ';
    }
    /* Assume preset list is not empty */
    l = strlen(builtin_presets[i].name);
    *(p++) = '"';
    memcpy(p, builtin_presets[i].name, l);
    p += l;
    *(p++) = '"';
    *(p++) = ' ';
    *(p++) = ']';
}

const char *preset_get(const char *name)
{
    int i;

    for (i = 0; builtin_presets[i].name != NULL; i++) {
        if (!strcmp(name, builtin_presets[i].name)) {
            return builtin_presets[i].data;
        }
    }

    return NULL;
}

#if 0
int main(void)
{
    int l;
    char *b;
    l = preset_list_len();
    b = malloc(l + 10);
    preset_list_fill(b);
    putchar('>');
    fwrite(b, l, 1, stdout);
    printf("<\n");
    printf(">%s<\n", preset_get("pulsate"));
    return 0;
}
#endif
