#ifndef _VFDM_H_
#define _VFDM_H_

#define VFD_SCROLLMAX 46

/* Play state, matching Winamp return values */
enum vfdm_playstate {
    VFDM_UNKNOWNSTATE = -1,
    VFDM_STOPPED = 0,
    VFDM_PLAYING = 1,
    VFDM_PAUSED = 3
};

#define    VFDM_TIMEOUT 1

int vfdm_init(const char *port);
void vfdm_close(void);
void vfdm_vu(unsigned int l, unsigned int r);
void vfdm_trackchange(int n, const char *name, unsigned int l);
// void vfdm_setplaystate(enum vfdm_playstate newstate); needs to go via thread
void vfdm_playstatechanged(void);
enum vfdm_playstate vfdm_cb_getplaystate(void);
void vfdm_pmsuspend(void);
void vfdm_pmwake(void);

int vfdm_cb_init(void (*threadmain)(void));
void vfdm_cb_close(void);
void vfdm_cb_settimer(unsigned int milliseconds);
void vfdm_cb_incrementtimer(unsigned int milliseconds);
unsigned int vfdm_cb_wait(void);
void vfdm_cb_signal(unsigned int flagstoset);

#endif /* ifndef _VFDM_H_ */
