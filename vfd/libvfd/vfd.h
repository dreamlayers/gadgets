#ifndef _VFD_H_

#ifdef __cplusplus
extern "C" {
#else

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

VFD_RETURN vfd_connect(const char *fname);
void vfd_disconnect(void);
VFD_RETURN vfd_clear(void);
VFD_RETURN vfd_full(void);

VFD_RETURN vfd_ulbmp(const unsigned short *s);
VFD_RETURN vfd_dlbmp(unsigned short *s);

void vfd_blitnstr(unsigned short buf[], const char *s, int n);
void vfd_blitstr(unsigned short buf[], const char *s);
void vfd_blitvu(unsigned short b[], int l, int r);
void vfd_blit7segdec(unsigned short b[], int i);
void vfd_blit7seghex(unsigned short b[], int i);

VFD_RETURN vfd_enterbm(void);
VFD_RETURN vfd_exitbm(void);

VFD_RETURN vfd_bms7dec(int i);
VFD_RETURN vfd_bms7hex(int i);

int vfd_bmreadadc(int i);
VFD_RETURN vfd_bmsetvu(int l, int r);
VFD_RETURN vfd_bmind(const unsigned char *ind);

VFD_RETURN vfd_bmclear(void);

VFD_RETURN vfd_bmsetscw(int s, int e);
VFD_RETURN vfd_bmntxt(unsigned int txm, const char *s, int n);
VFD_RETURN vfd_bmtxt(unsigned int txm, const char *s);
VFD_RETURN vfd_bmsetc(int p, char c);
VFD_RETURN vfd_bmnsets(int p, const char *s, int n);
VFD_RETURN vfd_bmsets(int p, const char *s);

VFD_RETURN vfd_bmparset(int n);
VFD_RETURN vfd_bmparon(int n);
VFD_RETURN vfd_bmparoff(int n);
VFD_RETURN vfd_bmparop(int n);
int vfd_bmreadpar(void);

void vfd_clearalarms(unsigned char *c);
void vfd_addalarm(unsigned char *c, int op, int par, int h, int m, int s, int r);
VFD_RETURN vfd_setclockto(int h, int m, int s, const unsigned char *al);
VFD_RETURN vfd_setclock(const unsigned char *al);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* !_VFD_H_ */
