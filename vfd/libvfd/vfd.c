/* Library for direct interfacing to the VFD display gadget. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "serio.h"
#define BUILDING_LIBVFD
#include "vfd.h"

/* Mapping of 14 segment text display segments to bits */
#define SD_TOP (1 << 7)
#define SD_LEFTUPPER (1 << 5)
#define SD_LEFTLOWER (1 << 2)
#define SD_RIGHTUPPER (1 << 6)
#define SD_RIGHTLOWER (1 << 3)
#define SD_BOTTOM (1 << 1)
#define SD_LEFT (1 << 4)
#define SD_RIGHT (1 << 13)
#define SD_UP (1 << 15)
#define SD_DOWN (1 << 11)
#define SD_LEFTUP (1 << 0)
#define SD_LEFTDOWN (1 << 10)
#define SD_RIGHTUP (1 << 14)
#define SD_RIGHTDOWN (1 << 12)
#define SD_DECIMAL (0)

/* Font for 14 segment text display */
#include "14.h"

/* Binary mode commands */
#define VFDBM_NOP     0
#define VFDBM_S7DEC   1
#define VFDBM_S7HEX   2
#define VFDBM_VUMODE  3
#define VFDBM_PRT     4
#define VFDBM_PRTLP   5
#define VFDBM_APPEND  6
#define VFDBM_APPENDL 7
#define VFDBM_ADC0    8
#define VFDBM_ADC1    9
#define VFDBM_ADC2    10
#define VFDBM_ADC3    11
#define VFDBM_PAROUT  12
#define VFDBM_IND     13
#define VFDBM_SETSCW  14
#define VFDBM_CLEAR   15
#define VFDBM_RDPAR   16

static int needsync; /* Sync needed to get back to command prompt */
static int vumode;   /* In VU bar graph mode within binary mode */

#ifdef VFD_ERRORS_FATAL
#if defined(__GNUC__)
static void fatalerr(char *s) __attribute__ ((noreturn));
#elif defined(_MSC_VER)
__declspec(noreturn)
#endif
static void fatalerr(char *s) {
#ifdef WIN32
  MessageBox(NULL, s, "VFD client", MB_OK | MB_ICONERROR);
#else
  fputs(s, stderr);
#endif
  exit(-1);
}

#define RETURN_IF_NONFATAL
#define VFD_ERROR(s) fatalerr(s)
#define VFD_SUCCESS() return
#define PROPAGATE_ERROR(f) f
#define RETURN_STATUS(f) f; return
#else
#define VFD_ERROR(s) return VFD_FAIL
#define VFD_SUCCESS() return VFD_OK
#define PROPAGATE_ERROR(f) { int res = (f); if (res != 0) return res; }
#define RETURN_STATUS(f) return f
#endif

/*
 * xvfd_* - Internal use only routines
 */

static int xvfd_sync(unsigned int tmout) {
    int c;
    PROPAGATE_ERROR(serio_setreadtmout(tmout));

    do {
        c = serio_getc();
        if (c < 0) return VFD_FAIL;
    } while (c != '>');

    needsync = 0;
    return VFD_OK;
}

static int xvfd_safesync(unsigned int n) {
  int ic;
  unsigned char c;

  if (xvfd_sync(n) == 0) {
    /* Sure we synced but we better not get any shit after the sync */
    c = '>';
    serio_setreadtmout(200);
    while ((ic = serio_getc()) >= 0) {
      c = ic;
    }

    if (c == '>') return 0;
  }
  return -1;
}

/* Get back to display command prompt from unknown state */
/* FIXME: This does not always work */
static int xvfd_recover(void) {
  int i;
  static const unsigned char recstr1[] = {
      VFDBM_NOP+0xF0,
      100,
      'q',
  };

  PROPAGATE_ERROR(serio_purge(SERIO_PURGEOUT | SERIO_PURGEIN));

  /* Wait for shit to stop coming in */
  PROPAGATE_ERROR(serio_setreadtmout(200));
  while (serio_getc() >= 0) {}
  /* This should have dealt with help, view, download and queued shit */

  /* Exit bytemode, loop, scroll (left) and draw */
  PROPAGATE_ERROR(serio_write(recstr1, sizeof(recstr1)));

  /* Long sync should deal with quitting loop, print, scroll */
  if (xvfd_safesync(2000) == 0) return 0;

  /* Okay, now do upload and scroll up */
  for (i = 0; i < 11; i++) {
    serio_write(recstr1, sizeof(recstr1));
  }
  serio_putc('\r');

  /* One last attempt to sync */
  return xvfd_safesync(3000);
}

/* Send normal command of specified length to display */
static VFD_RETURN xvfd_ncommand(const char *s, int l) {
    if (needsync) {
        if (xvfd_sync(2000) != 0) {
            printf("Initiating recovery!\n");
            if (xvfd_recover() != 0) {
                VFD_ERROR("Failed to recover, exiting.\n");
            }
        }
    }

    PROPAGATE_ERROR(serio_write((unsigned char *)s, l));
    RETURN_STATUS(serio_putc('\r'));
}

/* Send normal command to display */
static VFD_RETURN xvfd_command(const char *s) {
    RETURN_STATUS(xvfd_ncommand(s, (int)strlen(s)));
}

/*
 * vfd_* - Overall sign manipulation routines
 */

/* Connect to VFD (FIXME merge with init) */
VFD_RETURN vfd_connect(const char *fname) {
    if (serio_connect(fname, 1200) != 0) {
        VFD_ERROR("vfd_connect: serio_connect failed");
    }

    RETURN_STATUS(xvfd_recover());
}

/* Disconnect from display */
void vfd_disconnect(void) {
    serio_disconnect();
}

/* Clear display */
VFD_RETURN vfd_clear(void) {
    PROPAGATE_ERROR(xvfd_ncommand("c", 1));
#ifdef WIN32
    Sleep(100);
#else
    usleep(100*1000);
#endif
    needsync = 1;
    VFD_SUCCESS();
}

/* Lamp test (turn on all segments) */
VFD_RETURN vfd_full(void) {
    PROPAGATE_ERROR(xvfd_ncommand("t", 1));
#ifdef WIN32
    Sleep(100);
#else
    usleep(100*1000);
#endif
    needsync = 1;
    VFD_SUCCESS();
}

/* Upload bitmap */
VFD_RETURN vfd_ulbmp(const unsigned short *s) {
    PROPAGATE_ERROR(xvfd_command("U"));
    RETURN_STATUS(serio_write((const unsigned char *)s, 34));
}

static VFD_RETURN xvfd_getcmdecho(char cmd) {
    unsigned char inbuf[3], expects[3];

    expects[0] = cmd;
    expects[1] = 13;
    expects[2] = 10;

    serio_flush();
    serio_setreadtmout(500); // Why is this long delay needed for enterbm?
    if (serio_read(inbuf, 3) != 3) {
        VFD_ERROR("vfd_getcmdecho: read error");
    }

    if (memcmp(inbuf, expects, 3) == 0) {
        VFD_SUCCESS();
    } else {
        VFD_ERROR("vfd_getcmdecho: unexpected input");
    }
}

/* Start downloading bitmap from display */
static VFD_RETURN xvfd_dlstart(void) {
    PROPAGATE_ERROR(xvfd_command("D"));

    serio_setreadtmout(100);

    RETURN_STATUS(xvfd_getcmdecho('D'));
}

/* Download bitmap */
VFD_RETURN vfd_dlbmp(unsigned short *s) {
    PROPAGATE_ERROR(xvfd_dlstart());
    if (serio_read((unsigned char *)s, 34) == 34) {
        VFD_SUCCESS();
    } else {
        VFD_ERROR("vfd_dlbmp: read error");
    }
}

/* Write string of specified length into display bitmap */
void vfd_blitnstr(unsigned short buf[], const char *s, int n) {
    int i;

    for (i = 0; i < 12; i++) {
        if (i < n) {
            if (s[i] >= ' ' /* && s[i] <= 127 */) {
                buf[15-i] = font14[s[i] - 32];
            } else {
                buf[15-i] = 0xFDFF;
            }
        } else {
            buf[15-i] = 0;
        }
    }
}

/* Write string into display bitmap */
void vfd_blitstr(unsigned short buf[], const char *s) {
    vfd_blitnstr(buf, s, (int)strlen(s));
}

#if 0
/* Inefficient code which loops to create value */

/* Table from number to VU bar graph position */
static const unsigned short xvfd_vutab[] = {
    1 << 7, 1 << 6, 1 << 15, 1 << 0, 1 << 14, 1 << 5, 1 << 4, 1 << 13,
    1 << 3, 1 << 12, 1 << 10, 1 << 11, 1 << 2, 1 << 1
};

/* Convert number into VU bar graph value FIXME */
static unsigned short xvfd_vufilled(int v) {
    unsigned short r = 0;

    if (v > 14) {
        v = 14;
    }

    for (v--; v >= 0; v--) {
        r |= xvfd_vutab[v];
    }

    return r;
}

/* Write left and right VU bar graph value into display bitmap */
void vfd_blitvu(unsigned short b[], int l, int r) {
    b[2] &= (1 << 9) | (1 << 8);
    b[3] &= (1 << 9) | (1 << 8);

    b[2] |= xvfd_vufilled(l);
    b[3] |= xvfd_vufilled(r);
}
#else
/* New table based code */

/* Table from value to filled VU bargraph */
#if 0
static const unsigned short xvfd_vufilledtab[] = {
    0x0000, 0x0080, 0x00C0, 0x80C0, 0x80C1, 0xC0C1, 0xC0E1, 0xC0F1,
    0xE0F1, 0xE0F9, 0xF0F9, 0xF4F9, 0xFCF9, 0xFCFD, 0xFCFF
};
#endif
/* Table from value to filled VU bargraph with left and right indicators */
static const unsigned short xvfd_vufilledtab[] = {
    0x0300, 0x0380, 0x03C0, 0x83C0, 0x83C1, 0xC3C1, 0xC3E1, 0xC3F1,
    0xE3F1, 0xE3F9, 0xF3F9, 0xF7F9, 0xFFF9, 0xFFFD, 0xFFFF
};

void vfd_blitvu(unsigned short b[], int l, int r) {
    b[2] |= xvfd_vufilledtab[l <= 14 ? l : 14];
    b[3] |= xvfd_vufilledtab[r <= 14 ? r : 14];
}
#endif

/* 7 segment table 0-9A-F for tens */
const unsigned short xvfd_7seghtab[] =
{ (1 << 0) | (1 << 11) | (1 << 15) | (1 << 12) | (1 << 14) | (1 << 13),
(1 << 15) | (1 << 14),
(1 << 0) | (1 << 15) | (1 << 10) | (1 << 12) | (1 << 13),
(1 << 0) | (1 << 15) | (1 << 14) | (1 << 10) | (1 << 13),
(1 << 11) | (1 << 15) | (1 << 14) | (1 << 10),
(1 << 0) | (1 << 11) | (1 << 10) | (1 << 14) | (1 << 13),              /* 5 */
(1 << 0) | (1 << 11) | (1 << 10) | (1 << 12) | (1 << 14) | (1 << 13),
(1 << 0) | (1 << 15) | (1 << 14),
(1 << 0) | (1 << 11) | (1 << 15) | (1 << 12) | (1 << 14) | (1 << 13) | (1 << 10),
(1 << 0) | (1 << 11) | (1 << 15) | (1 << 14) | (1 << 13) | (1 << 10),
(1 << 0) | (1 << 11) | (1 << 15) | (1 << 12) | (1 << 14) | (1 << 10),  /* A */
(1 << 11) | (1 << 10) | (1 << 12) | (1 << 14) | (1 << 13),             /* b */
(1 << 0) | (1 << 11) | (1 << 12) | (1 << 13),                          /* C */
(1 << 15) | (1 << 10) | (1 << 12) | (1 << 14) | (1 << 13),             /* d */
(1 << 0) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 10),              /* E */
(1 << 0) | (1 << 11) | (1 << 12) | (1 << 10)                           /* F */
};

/* 7 segment table 0-9A-F for ones */
const unsigned short xvfd_7segltab[] = {
    (1 << 7) | (1 << 5) | (1 << 6) | (1 << 2) | (1 << 3) | (1 << 1),
    (1 << 6) | (1 << 3),
    (1 << 7) | (1 << 6) | (1 << 4) | (1 << 2) | (1 << 1),
    (1 << 7) | (1 << 6) | (1 << 3) | (1 << 4) | (1 << 1),
    (1 << 5) | (1 << 6) | (1 << 3) | (1 << 4),
    (1 << 7) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 1),              /* 5 */
    (1 << 7) | (1 << 5) | (1 << 4) | (1 << 2) | (1 << 3) | (1 << 1),
    (1 << 7) | (1 << 6) | (1 << 3),
    (1 << 7) | (1 << 5) | (1 << 6) | (1 << 2) | (1 << 3) | (1 << 1) | (1 << 4),
    (1 << 7) | (1 << 5) | (1 << 6) | (1 << 3) | (1 << 1) | (1 << 4),
    (1 << 7) | (1 << 5) | (1 << 6) | (1 << 2) | (1 << 3) | (1 << 4),  /* A */
    (1 << 5) | (1 << 4) | (1 << 2) | (1 << 3) | (1 << 1),             /* b */
    (1 << 7) | (1 << 5) | (1 << 2) | (1 << 1),                        /* C */
    (1 << 6) | (1 << 4) | (1 << 2) | (1 << 3) | (1 << 1),             /* d */
    (1 << 7) | (1 << 5) | (1 << 2) | (1 << 1) | (1 << 4),             /* E */
    (1 << 7) | (1 << 5) | (1 << 2) | (1 << 4)                         /* F */
};

/* Write 7 segment decimal value into display bitmap */
void vfd_blit7segdec(unsigned short b[], int i) {
    b[1] &= (1 << 9) | (1 << 8);

    b[1] |= xvfd_7segltab[i % 10];
    if (i > 9) b[1] |= xvfd_7seghtab[(i / 10) % 10];
}

/* Write 7 segment hex value into display bitmap */
void vfd_blit7seghex(unsigned short b[], int i) {
    b[1] &= (1 << 9) | (1 << 8);

    b[1] |= xvfd_7segltab[i & 0xF];
    if (i > 15) b[1] |= xvfd_7seghtab[(i >> 4) & 0xF];
}

/* Enter binary mode */
VFD_RETURN vfd_enterbm(void) {
    PROPAGATE_ERROR(xvfd_ncommand("B", 1));

    vumode = 0;

    RETURN_STATUS(xvfd_getcmdecho('B'));
}

/* Leave binary mode */
VFD_RETURN vfd_exitbm(void) {
    needsync = 1;

    if (vumode) {
        static const unsigned char vubmexit[2] = { VFDBM_NOP+0xF0, 100 };
        RETURN_STATUS(serio_write(vubmexit, 2));
    } else {
        RETURN_STATUS(serio_putc(100));
    }
}

/* Byte/binary mode command */
static VFD_RETURN xvfd_bmcmd2(unsigned char cmd, int data) {
    unsigned char buf[2];

    if (!vumode) {
        buf[0] = cmd;
    } else {
        buf[0] = cmd + 0xF0;
    }

    if (data >= 0) {
        buf[1] = (unsigned char)data;
        PROPAGATE_ERROR(serio_write(buf, 2));
    } else {
        PROPAGATE_ERROR(serio_putc(buf[0]));
    }

    if (cmd == VFDBM_NOP) {
        vumode = 0;
    }

    VFD_SUCCESS();
}

#define xvfd_bmcmd(cmd) xvfd_bmcmd2(cmd, -1)

/* Decimal display on 2x7seg */
VFD_RETURN vfd_bms7dec(int i) {
    RETURN_STATUS(xvfd_bmcmd2(VFDBM_S7DEC, i));
}

/* Hexadecimal display on 2x7seg */
VFD_RETURN vfd_bms7hex(int i) {
    RETURN_STATUS(xvfd_bmcmd2(VFDBM_S7HEX, i));
}

/* Read one specific ADC channel */
int vfd_bmreadadc(int i) {
    PROPAGATE_ERROR(xvfd_bmcmd(VFDBM_ADC0 + (i & 3)));
    serio_flush();
    serio_setreadtmout(200);
    return serio_getc();
}

/* Set left and right VU bar graphs */
VFD_RETURN vfd_bmsetvu(int l, int r) {
    unsigned char buf[2];

    if (l > 14) l = 14;
    if (r > 14) r = 14;
    if (l < 0) l = 0;
    if (r < 0) r = 0;

    buf[1] = (l << 4) | r;

    if (!vumode) {
        buf[0] = VFDBM_VUMODE;
        PROPAGATE_ERROR(serio_write(buf, 2));
        vumode = 1;
        VFD_SUCCESS();
    } else {
        RETURN_STATUS(serio_putc(buf[1]));
    }
}

/* Upload 5 bytes of data for indicators */
VFD_RETURN vfd_bmind(const unsigned char *ind) {
    PROPAGATE_ERROR(xvfd_bmcmd(VFDBM_IND));
    RETURN_STATUS(serio_write(ind, 5));
}

/* Clear display */
VFD_RETURN vfd_bmclear() {
    RETURN_STATUS(xvfd_bmcmd(VFDBM_CLEAR));
}

/* Set scroll window */
VFD_RETURN vfd_bmsetscw(int s, int e) {
    if (s < 0 || e < s || e > 12) {
        VFD_ERROR("Bad scroll window");
    }

    RETURN_STATUS(xvfd_bmcmd2(VFDBM_SETSCW,
                              ((12-s) << 4) | (12-e)));
}

/* Print specific length of text */
VFD_RETURN vfd_bmntxt(unsigned int txm, const char *s, int n) {
    char cmd;
    int i;
    unsigned char buf[46];

    if (n > 0) {
        switch (txm) {
        default:
        case 0: cmd = VFDBM_PRT; break;
        case VFDTXT_LOOP: cmd = VFDBM_PRTLP; break;
        case VFDTXT_APPEND: cmd = VFDBM_APPEND; break;
        case (VFDTXT_APPEND+VFDTXT_LOOP): cmd = VFDBM_APPENDL; break;
        }

        if (n > sizeof(buf)) n = sizeof(buf);

        for (i = 0; i < (n-1); i++) {
            buf[i] = s[i] & 0x7F;
        }
        buf[n-1] = s[n-1] | 0x80;
    } else {
        /* Appending nothing does nothing */
        if (txm & VFDTXT_APPEND) VFD_SUCCESS();
        /* Printing nothing clears the text area */
        cmd = VFDBM_PRT;
        buf[0] = ' ' | 0x80;
        n = 1;
    }

    PROPAGATE_ERROR(xvfd_bmcmd(cmd));
    RETURN_STATUS(serio_write(buf, n));
}


/* Print text without specifying length */
VFD_RETURN vfd_bmtxt(unsigned int txm, const char *s) {
    RETURN_STATUS(vfd_bmntxt(txm, s, (int)strlen(s)));
}

/* Put character at specific location */
VFD_RETURN vfd_bmsetc(int p, char c) {
    int v = 0;

    if (p < 1 || p > 12) {
        VFD_ERROR("vfd_bmsetc: position out of range");
    }

    if (vumode) {
        PROPAGATE_ERROR(xvfd_bmcmd(VFDBM_NOP));
    }

    if (c >= '0' && c <= '9') {
        v = c - '0';
    } else if (c == ' ') {
        v = 10;
    } else {
        unsigned char buf[2];
        buf[0] = 112 + (p-1) * 12 + 11;
        buf[1] = c;
        RETURN_STATUS(serio_write(buf, 2));
    }

    RETURN_STATUS(serio_putc(112 + (p-1) * 12 + v));
}

/* Put string of specified length at specific location */
VFD_RETURN vfd_bmnsets(int p, const char *s, int n) {
    int i;

    if (p < 1 || p > 12) {
        VFD_ERROR("vfd_bmnsets: position out of range");
    }

    for (i = 0; i < n && (i+p) < 12; i++) {
        PROPAGATE_ERROR(vfd_bmsetc(i+p, s[i]));
    }
    VFD_SUCCESS();
}

/* Put string at specific location */
VFD_RETURN vfd_bmsets(int p, const char *s) {
    RETURN_STATUS(vfd_bmnsets(p, s, (int)strlen(s)));
}

/* Parallel output operation */
VFD_RETURN vfd_bmparop(int n) {
    RETURN_STATUS(xvfd_bmcmd2(VFDBM_PAROUT, n & 0xFF));
}

/* Set all parallel outputs */
VFD_RETURN vfd_bmparset(int n) {
    RETURN_STATUS(vfd_bmparop(VFDPAR_SET | n));
}

/* Turn on specific parallel outputs */
VFD_RETURN vfd_bmparon(int n) {
    RETURN_STATUS(vfd_bmparop(0x3F & n));
}

/* Turn off specific parallel outputs */
VFD_RETURN vfd_bmparoff(int n) {
    RETURN_STATUS(vfd_bmparop(VFDPAR_OFF | (n & 0xBF)));
}

/* Perform operation with parallel outputs */
static int xvfd_bmparval(int op, int n) {
    switch (op) {
    case VFDPAR_OFF: return 0x80 | (n & 0xBF);
    case VFDPAR_ON: return 0x40 | (n & 0x7F);
    case VFDPAR_SET: return 0xC0 | n;
    default: VFD_ERROR("Invalid parallel output operation");
    }
}

/* Read parallel outputs */
int vfd_bmreadpar() {
    PROPAGATE_ERROR(xvfd_bmcmd(VFDBM_RDPAR));
    serio_flush();
    serio_setreadtmout(200);
    return serio_getc();
}

/* Clear alarm buffer. Call this before setting a new set of alarms */
void vfd_clearalarms(unsigned char *c) {
    c[0] = 0;
}

static int xvfd_tobcd(int n) {
    return (n % 10) | ((n / 10) << 4);
}

/* Append an alarm to an alarm buffer */
void vfd_addalarm(unsigned char *c, int op, int par, int h, int m, int s, int r) {
    unsigned char *p;

    p = c;
    while (*p != 0) {
        p += 4;
    }

    *(p++) = xvfd_bmparval(op, par);


    *(p++) = xvfd_tobcd(h);
    *(p++) = xvfd_tobcd(m);
    if (r) {
        *(p++) = xvfd_tobcd(s);
    } else {
        *(p++) = xvfd_tobcd(s) | 0x80;
    }

    *(p++) = 0;
}

/* Set clock to specified time with specified alarms */
/* FIXME: handle errors */
VFD_RETURN vfd_setclockto(int h, int m, int s, const unsigned char *al) {
    char buf[13];
    const unsigned char *p;

#ifdef _MSC_VER
    sprintf_s
#else
    snprintf
#endif
    (buf, sizeof(buf), "p%.2i %.2i %.2i", h, m, s);

    PROPAGATE_ERROR(xvfd_command(buf));
#ifdef WIN32
    Sleep(100);
#else
    usleep(100*1000);
#endif
    xvfd_ncommand("K", 1);
    serio_putc(xvfd_tobcd(h));
    serio_putc(xvfd_tobcd(m));
    serio_putc(xvfd_tobcd(s));

    if (al != NULL) {
        p = al;
        while (*p != 0) {
            serio_write(p, 4);
            p += 4;
        }
    }
    serio_putc(0);


    serio_getc(); /* 'K' */
    serio_getc(); /* 13 */
    serio_getc(); /* 10 */

    serio_getc(); /* Alarm byte count */
    //  printf("%.2i:%.2i:%.2i   al=%i\n", h, m, s, xvfd_getc());
    VFD_SUCCESS();
}

/* Set clock to current time and activate specified alarms */
VFD_RETURN vfd_setclock(const unsigned char *al) {
    time_t tmt;
#ifdef _MSC_VER
    struct tm ctm;
    time(&tmt);
    localtime_s(&ctm, &tmt);
    RETURN_STATUS(vfd_setclockto(ctm.tm_hour, ctm.tm_min, ctm.tm_sec, al));
#else
    struct tm *ctm;
    time(&tmt);
    ctm = localtime(&tmt);
    RETURN_STATUS(vfd_setclockto(ctm->tm_hour, ctm->tm_min, ctm->tm_sec, al));
#endif
}

VFD_RETURN vfd_flush(void) {
    if (serio_flush() == SERIO_OK) {
        VFD_SUCCESS();
    } else {
        VFD_ERROR("failed to flush serial port");
    }
}

int vfd_need_keepalive(void) {
    return serio_is_tcp();
}

int vfd_call_keepalive(void) {
    /* TODO: handle byte mode keepalive */
    static const unsigned char keepalive[] = " \b";
    return serio_write(keepalive, sizeof(keepalive));
}

#ifdef TESTING
int main(int argc, char **argv) {
  unsigned long i;

  /* Initialize LED sign */
#ifdef WIN32
  vfd_connect("COM1");
#elif defined(__CYGWIN__) || defined(__linux)
  vfd_connect("/dev/ttyS0");
#else
  vfd_connect("/dev/cu.serial1");
#endif

        printf("enterbm=>\n");
        printf("enterbm: %i\n", vfd_enterbm());
        vfd_bmtxt(1, "Testing...");
        for (i = 0; i < 99; i++) {
            vfd_bms7dec(i);
        }
        printf("bmreadadc(0): %i\n", vfd_bmreadadc(0));


  vfd_bmsetc(2,'5');

  vfd_bmsetscw(4,12);
  vfd_bmtxt(VFDTXT_LOOP, "TEST");

  vfd_bmparon(3);

  vfd_bmparoff(0xFE);

  printf("vfd_bmreadpar() = %i\n", vfd_bmreadpar());

  sleep(1);

  vfd_exitbm();

  vfd_disconnect();

  return 0;
}
#endif
