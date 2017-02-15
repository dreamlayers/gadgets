/* Header file for direct access library for the VFD gadget. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#ifndef _VFD_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(__CYGWIN__)
#ifdef BUILDING_LIBVFD
#ifdef BUILDING_DLL
#define VFDAPI __declspec(dllexport)
#else
#define VFDAPI
#endif
#else
/* This breaks static linking against libvfd.
 * #define VFDAPI __declspec(dllimport)
 */
#define VFDAPI
#endif
#else /* !defined(WIN32) && !defined(__CYGWIN__) */
#define VFDAPI
#endif

/* VBMI_LEFTARROW_B

<, unused, ), =, (, MUTE, PLAY, RECORD
RENUMBER, COUNTER, TRACK REMAIN, TRACK ELAPSED, TOTAL REMAIN, TOTAL ELAPSED, >, ||
-dB\nMAX LEVEL, right dB scale, RIGHT, left dB scale, LEFT, INDEX, OPTICAL, DIGITAL
TRACK, REVERSE, START, SEC, MIN, HR, AUTO, PROGRAM
ID(box), SKIP, unused, unused, INPUT(box), ANALOG, unused, unused */

#define VFDI_PLAY_A 0
#define VFDI_PLAY_B (1 << 6)
#define VFDI_RIGHT_A 1
#define VFDI_RIGHT_B (1 << 6)
#define VFDI_PAUSE_A 1
#define VFDI_PAUSE_B (1 << 7)
#define VFDI_LEFTVU_A 2
#define VFDI_LEFTVU_B (1 << 4)
#define VFDI_RIGHTVU_A 2
#define VFDI_RIGHTVU_B (1 << 2)
#define VFDI_SEC_A 3
#define VFDI_SEC_B (1 << 3)
#define VFDI_MIN_A 3
#define VFDI_MIN_B (1 << 4)
#define VFDI_HR_A 3
#define VFDI_HR_B (1 << 5)

#define VFDTXT_LOOP 1
#define VFDTXT_APPEND 2
#define VFDTXT_MAXCHARS 46

#ifdef VFD_ERRORS_FATAL
#define VFD_RETURN void
#else
#define VFD_OK 0
#define VFD_FAIL -1
#define VFD_RETURN int
#endif

/* Valid for alarms or byte mode. 0 is also on for byte mode */
#define VFDPAR_OFF     0x80
#define VFDPAR_ON      0x40
#define VFDPAR_SET     0xC0

VFDAPI VFD_RETURN vfd_connect(const char *fname);
VFDAPI void vfd_disconnect(void);
VFDAPI VFD_RETURN vfd_clear(void);
VFDAPI VFD_RETURN vfd_full(void);

VFDAPI VFD_RETURN vfd_ulbmp(const unsigned short *s);
VFDAPI VFD_RETURN vfd_dlbmp(unsigned short *s);

VFDAPI void vfd_blitnstr(unsigned short buf[], const char *s, int n);
VFDAPI void vfd_blitstr(unsigned short buf[], const char *s);
VFDAPI void vfd_blitvu(unsigned short b[], int l, int r);
VFDAPI void vfd_blit7segdec(unsigned short b[], int i);
VFDAPI void vfd_blit7seghex(unsigned short b[], int i);

VFDAPI VFD_RETURN vfd_enterbm(void);
VFDAPI VFD_RETURN vfd_exitbm(void);

VFDAPI VFD_RETURN vfd_bms7dec(int i);
VFDAPI VFD_RETURN vfd_bms7hex(int i);

VFDAPI int vfd_bmreadadc(int i);
VFDAPI VFD_RETURN vfd_bmsetvu(int l, int r);
VFDAPI VFD_RETURN vfd_bmind(const unsigned char *ind);

VFDAPI VFD_RETURN vfd_bmclear(void);

VFDAPI VFD_RETURN vfd_bmsetscw(int s, int e);
VFDAPI VFD_RETURN vfd_bmntxt(unsigned int txm, const char *s, int n);
VFDAPI VFD_RETURN vfd_bmtxt(unsigned int txm, const char *s);
VFDAPI VFD_RETURN vfd_bmsetc(int p, char c);
VFDAPI VFD_RETURN vfd_bmnsets(int p, const char *s, int n);
VFDAPI VFD_RETURN vfd_bmsets(int p, const char *s);

VFDAPI VFD_RETURN vfd_bmparset(int n);
VFDAPI VFD_RETURN vfd_bmparon(int n);
VFDAPI VFD_RETURN vfd_bmparoff(int n);
VFDAPI VFD_RETURN vfd_bmparop(int n);
VFDAPI int vfd_bmreadpar(void);

VFDAPI void vfd_clearalarms(unsigned char *c);
VFDAPI void vfd_addalarm(unsigned char *c, int op, int par,
                         int h, int m, int s, int r);
VFDAPI VFD_RETURN vfd_setclockto(int h, int m, int s,
                                 const unsigned char *al);
VFDAPI VFD_RETURN vfd_setclock(const unsigned char *al);

VFDAPI VFD_RETURN vfd_flush(void);

VFDAPI int vfd_need_keepalive(void);
VFDAPI VFD_RETURN vfd_call_keepalive(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* !_VFD_H_ */
