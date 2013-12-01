/* Based on Imager-0.42, a Perl extension for Generating 24 bit Images

Here is the original copyright:

================================================================
Copyright (c) 1999-2001 Arnar M. Hrafnkelsson. All rights reserved.
This program is free software; you can redistribute it and/or
modify it under the same terms as Perl itself.
================================================================

>> THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY WHATSOEVER <<
If you like or hate Imager, please let us know by sending mail
to imager@imager.perl.org

================================================================

Modified by Boris Gjenero.
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include "serio.h"
#define IN_LIBSIGN
#include "libsign.h"
#include "lsd.h"


static void set_logfont(char *face, int size, LOGFONT *lf) {
  memset(lf, 0, sizeof(LOGFONT));

  lf->lfHeight = -size; /* character height rather than cell height */
  lf->lfCharSet = ANSI_CHARSET;
  lf->lfOutPrecision = OUT_TT_PRECIS;
  lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lf->lfQuality = PROOF_QUALITY;
  strcpy_s(lf->lfFaceName, sizeof(lf->lfFaceName), face);
  /* NUL terminated by the memset at the top */
}


static LPVOID render_text(char *face, int size, char *text, int length, int aa,
           HBITMAP *pbm, SIZE *psz, TEXTMETRIC *tm) {
  BITMAPINFO bmi;
  BITMAPINFOHEADER *bmih = &bmi.bmiHeader;
  HDC dc, bmpDc;
  LOGFONT lf;
  HFONT font, oldFont;
  SIZE sz;
  HBITMAP bm, oldBm;
  LPVOID bits;

  dc = GetDC(NULL);
  set_logfont(face, size, &lf);

#ifdef ANTIALIASED_QUALITY
  /* See KB article Q197076
     "INFO: Controlling Anti-aliased Text via the LOGFONT Structure"
  */
  lf.lfQuality = aa ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
#endif

  bmpDc = CreateCompatibleDC(dc);
  if (bmpDc) {
    font = CreateFontIndirect(&lf);
    if (font) {
      oldFont = (HFONT)SelectObject(bmpDc, font);
      GetTextExtentPoint32(bmpDc, text, length, &sz);
      GetTextMetrics(bmpDc, tm);

      memset(&bmi, 0, sizeof(bmi));
      bmih->biSize = sizeof(*bmih);
      bmih->biWidth = sz.cx;
      bmih->biHeight = -sz.cy;
      bmih->biPlanes = 1;
      bmih->biBitCount = 1;  /* bpp */
      bmih->biCompression = BI_RGB;
      bmih->biSizeImage = 0;
      bmih->biXPelsPerMeter = (LONG)(72 / 2.54 * 100);
      bmih->biYPelsPerMeter = bmih->biXPelsPerMeter;
      bmih->biClrUsed = 0;
      bmih->biClrImportant = 0;

      bm = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);

      if (bm) {
    oldBm = (HBITMAP)SelectObject(bmpDc, bm);
    SetTextColor(bmpDc, RGB(255, 255, 255));
    SetBkColor(bmpDc, RGB(0, 0, 0));
    TextOut(bmpDc, 0, 0, text, length);
    SelectObject(bmpDc, oldBm);
      }
      else {
    fprintf(stderr, "Could not create DIB section for render: %ld",
              GetLastError());
    SelectObject(bmpDc, oldFont);
    DeleteObject(font);
    DeleteDC(bmpDc);
    ReleaseDC(NULL, dc);
    return NULL;
      }
      SelectObject(bmpDc, oldFont);
      DeleteObject(font);
    }
    else {
      fprintf(stderr, "Could not create logical font: %ld",
            GetLastError());
      DeleteDC(bmpDc);
      ReleaseDC(NULL, dc);
      return NULL;
    }
    DeleteDC(bmpDc);
  }
  else {
    fprintf(stderr, "Could not create rendering DC: %ld", GetLastError());
    ReleaseDC(NULL, dc);
    return NULL;
  }

  ReleaseDC(NULL, dc);

  *pbm = bm;
  *psz = sz;

  return bits;
}

int getcol(unsigned char *bmp, int line_width, int x, int y) {
  int i, v;
  unsigned char *p, m;

  p = bmp + line_width * y + x / 8;
  v = 0;
  m = 0x80 >> (x % 8);

  for (i = 0; i < 7; i++) {
    v <<= 1;
    if (*p & m) v |= 1;
    p += line_width;
  }

  return v;
}

int
i_wf_text(char *face, int size,
      char *text, int len, scmdblk *scb) {
  unsigned char *bits, cc;
  HBITMAP bm;
  SIZE sz;
  int line_width;
  int x, ledx;
  int dontquit = 0x80, ntfy = 0;
  TEXTMETRIC tm;

  bits = (unsigned char *)render_text(face, size, text, len, 0, &bm, &sz, &tm);
  if (!bits)
    return 0;

  line_width = (sz.cx / 8) + ((sz.cx % 8) ? 1 : 0);
  line_width = (line_width + 3) / 4 * 4;

/*  top = ty;
  if (align)
    top -= tm.tmAscent; */

#if 0
  for (y = 0; y < sz.cy; ++y) {
    for (x = 0; x < sz.cx; ++x) {
       if ((bits[x/8] >> (7 - (x % 8))) & 1) {
         putchar('*');
       } else {
         putchar('.');
       }
    }
    bits += line_width;
    printf("\n");
  }
#endif

  /* Find end */
  if (!(scb->flags & SFLAG_LOOP)) {
    for (;sz.cx >= 0; --sz.cx) {
      if (getcol(bits, line_width, sz.cx-1, 1) != 0) break;
    }
  } else {
    ntfy = 1;
  }

  if (scb->flags & SFLAG_APPEND) {
    scrl_start();
    ledx = 72;
  } else {
    xsign_command(SIGNCMD_BYTEMODE);  /* Byte mode */
    serio_putc(192);     /* Begin upload at column 0 */
    ledx = 0;
  }

  sz.cx--;
  do {
    x = 0;
    do {
      cc = getcol(bits, line_width, x, 1);

      if (xsign_pollquit()) {
        sz.cx = x;
        dontquit = 0;
      } else if (x == sz.cx && !(scb->flags & SFLAG_LOOP)) {
        dontquit = 0;
      }

      if (ledx == 0 && cc == 0) {
        continue;
      } else {
        if (ledx < 71) {
          serio_putc(cc | dontquit);
          ledx++;
        } else if (ledx == 71) {
          serio_putc(cc);
          serio_putc('X');     /* Exit byte mode */
          scrl_start();
          ledx++;
        } else {
          serio_putc(cc | dontquit);
        }
      }

      x++;
    } while (x <= sz.cx);

    /* Notify after one completion */
    if (ntfy) {
      if (scb->event != NULL) {
        scb->result = 0;
        SetEvent(scb->event);
      }
      ntfy = 0;
    }

  } while (dontquit && (scb->flags & SFLAG_LOOP));

  if (ledx < 72)
    serio_putc('X');     /* Exit byte mode */

  DeleteObject(bm);

  return 1;
}

#if 0
int main(int argc, char **argv) {
  char *s = "123";
  char *f = "Lesspixels";

  if (argc >= 2) s = argv[1];

  return i_wf_text(f, 7, s, strlen(s), 0);
}
#endif
