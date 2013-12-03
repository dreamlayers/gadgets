/* libsign.h */

#ifndef _LIBSIGN_H_
#define _LIBSIGN_H_

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(__CYGWIN__)
#ifdef IN_LIBLEDSIGN
#define LEDAPI __declspec(dllexport)
#else
/* This breaks static linking.
 * #define LEDAPI __declspec(dllimport)
 */
#define LEDAPI
#endif
#else /* !defined(WIN32) && !defined(__CYGWIN__) */
#define LEDAPI
#endif

/* enum sign_drawop_e { COPY, XOR, AND, OR };*/

typedef struct {
  unsigned char data[256*8];
  unsigned int height;
  unsigned int width;
} sign_font_t;

LEDAPI int sign_open(const char *fname);
LEDAPI void sign_close(void);
LEDAPI int sign_clear(void) ;
LEDAPI int sign_full(void);

#define SIGN_HWP_APPEND 1
#define SIGN_HWP_LOOP 2

LEDAPI int sign_nhwprint(const char *s, size_t n, unsigned int hwpflags);
LEDAPI /* int sign_hwprint(const char *s, int hwpflags); */

LEDAPI int sign_loadfont(const char *fname, sign_font_t *font);
#ifdef WIN32
LEDAPI int sign_fontfromresource(WORD resid, sign_font_t *font);
#endif

typedef int font_style;

#define SIGN_FONT_INVERTED 1
#define SIGN_FONT_BOLD 2
#define SIGN_FONT_ITALIC 4

LEDAPI int sign_scrl_start(void);
LEDAPI int sign_scrl_char(const char c, sign_font_t *font, font_style fs,
                          int chain);
LEDAPI int sign_scrl_str(const char *s, sign_font_t *font, font_style fs);
LEDAPI int sign_scrl_nstr(const char *s, size_t n,
                          sign_font_t *font, font_style fs);

LEDAPI int sign_scru_str(const char *s, sign_font_t *font, font_style fs);
LEDAPI int sign_scru_nstr(const char *s, size_t n,
                          sign_font_t *font, font_style fs);

LEDAPI int sign_ul_str(const char *s, sign_font_t *font, font_style fs);
LEDAPI int sign_ul_nstr(const char *s, size_t n,
                        sign_font_t *font, font_style fs);

LEDAPI int sign_ul_bmap(const unsigned char *s);

LEDAPI int sign_dl_bmap(unsigned char *s);
LEDAPI int sign_dl_represent(FILE *f);

#ifdef IN_LIBLEDSIGN
typedef enum {
    SIGNCMD_CLEAR,
    SIGNCMD_FULL,
    SIGNCMD_UL,
    SIGNCMD_DL,
    SIGNCMD_SCRL,
    SIGNCMD_SCRU,
    SIGNCMD_BYTEMODE
} signcmd_t;

int xsign_pollquit(void);
int xsign_command(signcmd_t cmd);
#endif /* IN_LIBSIGN */

LEDAPI void sign_setabortpoll(int (*func)(void));

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _LIBLEDSIGN_H_ */
