#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>
#endif
#include "serio.h"
#define IN_LIBLEDSIGN
#include "ledsign.h"

static int (*abortproc)(void) = NULL;

volatile static int interruptable = 0;

/* Ways of getting back to the command prompt */
typedef enum {
    SIGNMODE_PROMPT,   /* At prompt: just send command */
    SIGNMODE_NEEDSYNC, /* Command prompt should appear with no activity */
    SIGNMODE_SEND1,    /* Send byte to exit scroll right or message looping */
    SIGNMODE_SENDRR,
    SIGNMODE_UNKNOWN
} signmode_t;

static signmode_t signmode = SIGNMODE_NEEDSYNC;

struct signcmd {
    char c;             /* Character sent to sign command line */
    unsigned int sleep; /* Sleep after command in milliseconds */
    signmode_t mode;    /* Resulting sign mode */
    int interruptable;  /* Is command interruptable */
};

/* Syncing removes need for waiting for all except scroll up, where the
 * data needs to be moved before new input is accepted for the bottom
 */
static const struct signcmd signcmdtab[] = {
    { 'c', 0, SIGNMODE_NEEDSYNC, 0 },
    { 't', 0, SIGNMODE_NEEDSYNC, 0 },
    { 'U', 0, SIGNMODE_UNKNOWN, 0 },
    { 'D', 0, SIGNMODE_UNKNOWN, 0 },
    { 'L', 0, SIGNMODE_SENDRR, 1 },
    { '^', 50, SIGNMODE_UNKNOWN, 0 },
    { 'B', 0, SIGNMODE_SENDRR, 1 }
};

#define PROPAGATE_ERROR(f) res = (f); if (res != 0) return res;

/*
 * xsign_* - Internal use only routines
 */

/* Poll for aborts from within libsign code */
int xsign_pollquit(void) {
    if (!interruptable || abortproc == NULL) {
        return 0;
    } else {
        return abortproc();
    }
}

/* Wait for sign command prompt.
 * There must not be any other '>' characters before the
 * command prompt we're waiting for
 */
static int xsign_sync(unsigned int tmout) {
    int res;
    PROPAGATE_ERROR(serio_setreadtmout(tmout));

    do {
        res = serio_getc();
        if (res < 0) return res;    /* Error */
    } while (res != '>');

    signmode = SIGNMODE_PROMPT;
    return SERIO_OK;
}

static int xsign_safesync(unsigned int tmout) {
    int ic;
    unsigned char c;

    if (xsign_sync(tmout) == 0) {
        /* Sure we synced but we better not get any shit after the sync */
        c = '>';
        serio_setreadtmout(200);
        while ((ic = serio_getc()) >= 0) {
            c = ic;
        }

        if (c == '>') return SERIO_OK;
    }
    return -1;
}

/* Move from unknown sign state to sign command prompt */
static int xsign_recover(void) {
    int res;
    unsigned char buf[64];

    PROPAGATE_ERROR(serio_purge(SERIO_PURGEIN | SERIO_PURGEOUT));

    /* Wait for shit to stop coming in */
    PROPAGATE_ERROR(serio_setreadtmout(200));
    while (serio_getc() >= 0) {} /* FIXME? Can't tell between error and timeout */
    /* This should have dealt with help, view, download and queued shit */

    /* Exit bytemode, loop, scroll (left), draw, upload and scroll up */
    memset(buf, 'q', sizeof(buf)-1);
    buf[63] = '\r';
    PROPAGATE_ERROR(serio_write(buf, sizeof(buf)));
    return xsign_safesync(3000);
}

static int xsign_cmdprep(void) {
    int res;
    static const unsigned char bytequit[] = "\r\r";

    switch (signmode) {
        case SIGNMODE_SENDRR:
            /* It doesn't matter if the command was submitted or not.
             * However, there can be an extra '>'
             */
            PROPAGATE_ERROR(serio_write(bytequit, 2));
            if (xsign_safesync(5000) == 0) return 0;
            break;
        case SIGNMODE_PROMPT:
            return 0;
        case SIGNMODE_SEND1:
            PROPAGATE_ERROR(serio_putc(0));
        case SIGNMODE_NEEDSYNC:
            res = xsign_sync(5000); /* TODO: set per-command timeouts */
            if (res == 0) return 0;
        default:
            break;  /* Unnecessary but avoids warning for SIGNMODE_UNKNOWN */
    }

    /* SIGNMODE_UNKNOWN */
    if (xsign_pollquit()) return -1;
    return xsign_recover();
}

int xsign_command(signcmd_t cmd) {
    int res;
    unsigned char buf[2];

    if (signcmdtab[cmd].interruptable == 0) {
        /* Going from one interruptable command to another
         * is interruptable.
         */
        interruptable = 0;
    }

    PROPAGATE_ERROR(xsign_cmdprep());

    buf[0] = signcmdtab[cmd].c;
    buf[1] = '\r';

    signmode = SIGNMODE_UNKNOWN; /* If we don't know if command was issued */
    PROPAGATE_ERROR(serio_write(buf, 2));

    if (signcmdtab[cmd].sleep > 0) {
#ifdef WIN32
        Sleep(signcmdtab[cmd].sleep);
#else
        usleep(signcmdtab[cmd].sleep * 1000);
#endif
    }

    signmode = signcmdtab[cmd].mode;

    interruptable = signcmdtab[cmd].interruptable;

    return 0;
}

/* Poll for aborts from within serio writes */
static int xsign_seriopollquit(void) {
    if (!interruptable || abortproc == NULL) {
        return 0;
    } else {
        int res = abortproc();
        if (res) serio_purge(SERIO_PURGEOUT);
        if (signmode == SIGNMODE_NEEDSYNC) signmode = SIGNMODE_SENDRR;
        return res;
    }
}

/*
 * sign_* - Overall sign manipulation routines
 */

int sign_open(const char *fname) {
    int res;

    interruptable = 0;
    serio_setabortpoll(xsign_seriopollquit);
    PROPAGATE_ERROR(serio_connect(fname, 600));

    /* Assume sign is in unknown state. Any '>' character could be fake. */
    return xsign_recover();
}

void sign_close(void) {
    serio_disconnect();
}

int sign_clear(void) {
    return xsign_command(SIGNCMD_CLEAR);
}

int sign_full(void) {
    return xsign_command(SIGNCMD_FULL);
}

/* Load font */
int sign_loadfont(const char *fname, sign_font_t *font) {
  FILE *ff;

#ifdef _MSC_VER
  if (fopen_s(&ff, fname, "rb") != 0) return -1;
#else
  ff = fopen(fname, "rb");
#endif
  if (ff == NULL) return -1;

  font->width = 6;
  font->height = 7;
  if (fread(font->data, 256, font->height, ff) != font->height) return -1;

  fclose(ff);
  return 0;
}

#ifdef WIN32
int sign_fontfromresource(WORD resid, sign_font_t *font)
{
    HGLOBAL     res_handle = NULL;
    HRSRC       res;
    char *      res_data;
    /* DWORD       res_size; */

    /* NOTE: providing g_hInstance is important, NULL might not work */
    res = FindResource(NULL, MAKEINTRESOURCE(resid), RT_RCDATA);
    if (!res) return -1;
    res_handle = LoadResource(NULL, res);
    if (!res_handle) return -1;
    res_data = (char*)LockResource(res_handle);
    if (SizeofResource(NULL, res) < 256) return -1;

    font->width = 6;
    font->height = 7;
    memcpy(font->data, res_data, 256 * font->height);
    return 0;

    /* Freeing and unloading the resource is impossible. */
}
#endif

void sign_setabortpoll(int (*func)(void)) {
    abortproc = func;
}

static unsigned char xsign_styledfnt(unsigned char c, int r, sign_font_t *font, font_style fs) {
    unsigned char d = font->data[c*font->height+r];
    int i;
    int a = 0, b = 0;

    if (fs & SIGN_FONT_ITALIC) {
        switch(r) {
            case 0:
            case 1:  d >>= 1; break;
            case 5:
            case 6:  d <<= 1; break;
        }
    }

    if (fs & SIGN_FONT_BOLD) {
        if (r > 0) a = font->data[c*font->height+r-1];
        /*     if (r < 6) b = font->data[c*font->height+r+1]; */
        for (i = 0x80; i > 0; i >>= 1) {
            if ((a & i) || (b & i)) d |= i;
            if (d & (i >> 1)) d |= i;
        }
    }

    if (fs & SIGN_FONT_INVERTED) {
        d ^= 0xFF << (8 - font->width);
    }

    return d;
}

/* Start left scroll */
int sign_scrl_start(void) {
    return xsign_command(SIGNCMD_SCRL);
}


/* Scroll left one character */
static int xscrl_char(const char c, sign_font_t *font, font_style fs, int chain) {
    int col, row, ob, sc[7];
    unsigned char buf[8];
    unsigned char *p = &buf[0];

    /* Generate styled character */
    for (row = 0; row < 7; row++) {
        sc[row] = xsign_styledfnt(c, row, font, fs);
    }

    /* Loop through columns */
    for (col = 7; col >= 8-(int)(font->width); col--) {
        ob = 1;
        /* Construct number for column */
        for (row = 0; row < 7; row++) {
            ob = (ob << 1) | ((sc[row] >> col) & 1);
        }
        *(p++) = ob;
    }

    if (!chain) *(p-1) &= 0x7F;
    return serio_write(&buf[0], font->width);
}

int sign_scrl_char(const char c, sign_font_t *font, font_style fs, int chain) {
    int res;
    PROPAGATE_ERROR(xscrl_char(c, font, fs, chain));
    if (!chain) signmode = SIGNMODE_NEEDSYNC;
    return 0;
}

int sign_scrl_nstr(const char *s, size_t n, sign_font_t *font, font_style fs) {
    size_t i;
    int res;

    if (n > 0) {
        n--;
        PROPAGATE_ERROR(sign_scrl_start());
        for (i = 0; i < n; i++) {
            PROPAGATE_ERROR(sign_scrl_char(s[i], font, fs, 1));
        }
        PROPAGATE_ERROR(sign_scrl_char(s[n], font, fs, 0));
    }
    return 0;
}

int sign_scrl_str(const char *s, sign_font_t *font, font_style fs) {
    if (s == NULL) return 0;
    return sign_scrl_nstr(s, strlen(s), font, fs);
}

static int xsign_sendrow(const char *s, size_t n, sign_font_t *font, font_style fs, unsigned int row) {
    unsigned int curchar = 0;    /* Current char in string */
    unsigned int ledgroup;
    unsigned int fontbitsleft = 0;
    unsigned char buf[9];
    unsigned char fontbyte = 0;

    /* Loop through all 9 LED groups */
    for (ledgroup = 0; ledgroup < 9; ledgroup++) {
        unsigned int bitsused;
        unsigned char curbyte;

        if (fontbitsleft > 0) {
            /* Put remaining bits of last character into first bits */
            /* Assume everything fits because font width <= 8 */
            curbyte = (fontbyte << (font->width - fontbitsleft)) & 0xFF;
            bitsused = fontbitsleft;
            fontbitsleft = 0;
        } else {
            curbyte = 0;
            bitsused = 0;
        }

        while (1) {
            if (curchar < n) {
                /* Still within string */
                fontbyte = xsign_styledfnt(s[curchar++], row, font, fs);
                /* Assume that unused least significant bits are 0 */
                curbyte |= fontbyte >> bitsused;
                bitsused += font->width;
                if (bitsused >= 8) {
                    /* Filled up the byte */
                    fontbitsleft = bitsused - 8;
                    break;
                }
            } else {
                /* Past end of string */
                if (fs & SIGN_FONT_INVERTED) {
                    /* When font is inverted, light all LEDs past end */
                    curbyte |= 0xFF >> bitsused;
                }
                break;
            }
        } /* while */

        buf[ledgroup] = curbyte;
    }
    return serio_write(buf, 9);
}

/* Scroll up */
int sign_scru_nstr(const char *s, size_t n, sign_font_t *font, font_style fs) {
    unsigned int row;
    int res;

    /* Loop through 7 rows of the font */
    for (row = 0; row < 7 && !xsign_pollquit(); row++) {
        PROPAGATE_ERROR(xsign_command(SIGNCMD_SCRU));
        PROPAGATE_ERROR(xsign_sendrow(s, n, font, fs, row));
        signmode = SIGNMODE_NEEDSYNC;
    }

    return 0;
}

int sign_scru_str(const char *s, sign_font_t *font, font_style fs) {
    size_t n;

    if (s == NULL) {
        n = 0;
    } else {
        n = strlen(s);
    }

    return sign_scru_nstr(s, n, font, fs);
}

/*
 * ul_* - Uploading routines
 */

#if 0
/* Upload string top->bottom */
int xxul_nstr(const char *s, int n, sign_font_t *font, font_style fs) {
    int row, res;

    PROPAGATE_ERROR(xsign_command(SIGNCMD_UL));

    /* Loop through 7 rows of the font */
    for (row = 0; row < 7 && !xsign_pollquit(); row++) {
        PROPAGATE_ERROR(xsign_sendrow(s, n, font, fs, row));
    }

    signmode = SIGNMODE_NEEDSYNC;
    return 0;
}
#endif

/* Upload string left->right */
int sign_ul_nstr(const char *s, size_t n, sign_font_t *font, font_style fs) {
    int i, cont, res;
    size_t lmt;

    lmt = 72 / font->width;

    if (n > lmt) {
        n = lmt;
    } else if (n == 0) {
        return 0;
    }
    cont = 1;

    PROPAGATE_ERROR(xsign_command(SIGNCMD_BYTEMODE));  /* Byte mode */
    PROPAGATE_ERROR(serio_putc(192));     /* Begin upload at column 0 */

    i = 0;
    while (1) {
        if (i == n - 1) {
            cont = 0;
        }
        PROPAGATE_ERROR(xscrl_char(s[i++], font, fs, cont));
        if (!cont) break;
        cont = !xsign_pollquit();
    }

    PROPAGATE_ERROR(serio_putc('X'));     /* Exit byte mode */
    signmode = SIGNMODE_NEEDSYNC;
    return 0;
}

int sign_ul_str(const char *s, sign_font_t *font, font_style fs) {
    size_t n;

    if (s == NULL) {
        n = 0;
    } else {
        n = strlen((const char *)s);
    }

    return sign_ul_nstr(s, n, font, fs);
}

#if 0
/* Upload string centre->left,right */
int sign_ul_nstr(const char *s, int n, sign_font_t *font, font_style fs) {
    int i, j, k, col, lmt, res;
    unsigned char cc[7], cols[72], buf[4];

    static int x = 0;

    lmt = 72 / font->width;

    if (n > lmt) {
        n = lmt;
    }

    PROPAGATE_ERROR(xsign_command(SIGNCMD_BYTEMODE));

    /* Pre-generate bitmap */
    col = 0;
    for (i = 0; i < n; i++) {
        /* Generate styled character */
        for (j = 0; j < 7;j++) {
            cc[j] = xsign_styledfnt(s[i], j, font, fs);
        }

        /* Put character into columns */
        for (j = 7; j >= (8-font->width); j--) {
            /* Construct number for column */
            cols[col] = 0;
            for (k = 0; k < 7; k++) {
                cols[col] = (cols[col] << 1) | ((cc[k] >> j) & 1);
            }
            col++;
        }
    }

    /* Zero remaining columns */
    for (; col < 72; col++) {
        cols[col] = 0;
    }

    for (i = 0; i < 36 /* 72/2 */ && !xsign_pollquit(); i++) {
        buf[0] = 157+i;     /* Before centre */
        buf[1] = cols[35-i];
        buf[2] = 156-i;     /* Past centre */
        buf[3] = cols[36+i];
        PROPAGATE_ERROR(serio_write(buf, 4));
    }

    PROPAGATE_ERROR(serio_putc('X'));     /* Exit byte mode */
    signmode = SIGNMODE_NEEDSYNC;
    return 0;
}
#endif

/* Upload bitmap */
int sign_ul_bmap(const unsigned char *s) {
    int res;
    PROPAGATE_ERROR(xsign_command(SIGNCMD_UL));
    PROPAGATE_ERROR(serio_write(s, 63));
    signmode = SIGNMODE_NEEDSYNC;
    return 0;
}

static int xsign_dlstart(void) {
    int res;

    PROPAGATE_ERROR(xsign_command(SIGNCMD_DL));

    PROPAGATE_ERROR(serio_setreadtmout(100));

    /* Now read everything after this */
    if (serio_getc() != 'D') return -1;
    if (serio_getc() != '\r') return -1;
    if (serio_getc() != '\n') return -1;

    return 0;
}

/* Download bitmap */
int sign_dl_bmap(unsigned char *s) {
    int res;
    PROPAGATE_ERROR(xsign_dlstart());
    /* FIXME: strict reads or this */
    res = (int)serio_read(s, 63);
    if (res < 0) return res;
    if (res != 63) return -1;
    signmode = SIGNMODE_NEEDSYNC;
    return 0;
}

#if 0
/* Write sign contents into file in human-readable format */
void sign_dl_represent(FILE *f) {
    int h, i, j, res;
    unsigned char c;

    PROPAGATE_ERROR(xsign_dlstart());

    for (h = 0; h < 7; h++) {
        for (i = 0; i < 9; i++) {
            c = serio_getc();
            for (j = 128; j > 0; j >>= 1) {
                putc(c&j?'.':'*', f);
            }
        }
        putc('\n', f);
    }
}
#endif

int sign_nhwprint(const char *s, size_t l, unsigned int hwpflags) {
#if SIGN_APPEND != 1 || SIGN_LOOP != 2
#error Flags mismatch with hwpcmd
#endif
    static const char hwpcmd[] = "psOo";
    int res;
    unsigned char buf[19];
    size_t usablel = (l < 17) ? l : 17;

    if (hwpflags > 4) return -1;

    interruptable = 0; /* FIXME */
    PROPAGATE_ERROR(xsign_cmdprep());
    signmode = SIGNMODE_UNKNOWN;
    buf[0] = hwpcmd[hwpflags];
    memcpy(&buf[1], s, usablel);
    buf[usablel+1] = '\r';

    PROPAGATE_ERROR(serio_write(buf, usablel+2));

    if (hwpflags & SIGN_LOOP) {
        signmode = SIGNMODE_SEND1;
    } else {
        signmode = SIGNMODE_NEEDSYNC;
    }

    return 0;
}

/* This requires bufl >= 1 */
static int xsign_formline(char *buf, size_t bufl, /* destination */
                          const char *s, unsigned int l, /* whole string */
                          const char **p, /* current position, updated */
                          size_t *remains, /* remaining, updated */
                          int flags) { /* flags, for looping */
    char *bufp = buf;
    size_t bufremains = bufl;
    const char *ptr = *p; /* Current pointer in string */

    /* Handle case where function is called at end of buffer */
    if (*remains == 0) {
        ptr = s;
        *remains = l;
    }

    while (1) {
        if (*ptr == '\n' || *ptr == '\r') /* End of line */
            break;

        *(bufp++) = *(ptr++); /* Copy character to buffer */

        if (--(*remains) == 0) {
            if (flags & SIGN_LOOP) {
                /* End of string, re-start from beginning */
                ptr = s;
                *remains = l;
            } else {
                /* No looping, finished */
                bufremains--;
                break;
            }
        }

        if (--bufremains == 0) /* Filled buffer */
            break;
    }

    *p = ptr; /* Update pointer. */
    return bufl - bufremains; /* Return length */
}

int sign_nswprint(const char *s, size_t l,
                  sign_font_t *font, font_style fs, unsigned int flags) {
    const char *p = s;
    size_t remains = l;
    int inscrl = 0;
    int res, qnow;
    char buf[72];
    int was_cr = 0;

    if (s == NULL || l == 0) {
        return 0; //xsign_printnull(flags);
    } else {
        p = s;
        remains = l;

        /* If not appending, upload first part left->right */
        if (!(flags & SIGN_APPEND)) {
            int len;

            /* How much to print? */
            len = xsign_formline(buf, 72 / font->width,
                                 s, l,
                                 &p, &remains, flags);

            if (len > 0) {
                /* Upload left -> right */
                PROPAGATE_ERROR(sign_ul_nstr(buf, len, font, fs));
            }
        }

        /* Loop for message looping */
        while (1) {
            /* Loop for going through message */
            while (remains > 0) {
                qnow = 0; //cmd_cb_pollquit();
                if (qnow && !inscrl) return 0;

                if (*p == '\r') {
                    p++;
                    remains--;

                    was_cr = 1;
                } else if (was_cr || *p == '\n') {
                    int len, was_nl;

                    was_nl = (*p == '\n');

                    if (was_nl) {
                        p++;
                        remains--;
                        /* Let xsign_formline() handle end of string wrap */
                    }

                    len = xsign_formline(buf, 72 / font->width,
                                         s, l,
                                         &p, &remains, flags);

                    if (was_nl) {
                        /* "\r\n" or "\n" */
                        PROPAGATE_ERROR(sign_scru_nstr(buf, len, font, fs));
                    } else {
                        /* Only "\r" */
                        PROPAGATE_ERROR(sign_ul_nstr(buf, len, font, fs));
                    }

                    was_cr = 0;
                } else {
                    /* Horizontal scrolling */
                    if (!inscrl) {
                        PROPAGATE_ERROR(sign_scrl_start());
                    }

                    /* Continue horizontal scroll mode if next character
                     * can be appended that way.
                     */
                    if (qnow) {
                        inscrl = 0;
                    } else if (remains == 1) {
                        /* Last character */
                        if (flags & SIGN_LOOP) {
                            /* Next character is first character in message.
                             * It must exist because remains >= l.
                             */
                            inscrl = (*s != '\r' && *s != '\n');
                        } else {
                            inscrl = 0;
                        }
                    } else /* remains > 1 */ {
                        inscrl = (*(p+1) != '\r' && *(p+1) != '\n');
                    }
                    res = sign_scrl_char(*p, font, fs, inscrl);
                    if (res != 0 || qnow) return res;
                    p++;
                    remains--;
                }
            } /* while (remains > 0) */

            if (flags & SIGN_LOOP) {
#if 0
                /* Notify after one completion */
                if (ntfy) {
                cmd_cb_notify(scb);
                ntfy = 0;
}
#endif
                /* Reset to start */
                p = s;
                remains = l;
            } else {
                /* No looping */
                break;
            }
        }
    }
    return 0;
}

int sign_swprint(const char *s,
                 sign_font_t *font, font_style fs, unsigned int flags) {
    return sign_nswprint(s, strlen(s), font, fs, flags);
}

#if 0
int main(void) {
    sign_font_t font;
    sign_open("COM9");
    sign_loadfont("..\\lsd\\6x7.rawfont", &font);
    sign_swprint("xyz\r123\nabc\n\nABC\r", &font, 0, SIGN_LOOP);
    return 0;
}
#endif
