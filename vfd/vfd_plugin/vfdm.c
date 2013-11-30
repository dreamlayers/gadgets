//#define __W32API_USE_DLLIMPORT__
//#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <time.h>
#include <string.h>

#include "vfd.h"
#include "sysmon.h"
#include "vfdm.h"
#ifdef ANALOGLOG
#include "analoglog.h"
#endif

/* (1 << 0) is timeout flag in vfdm.h */
#define VFDM_FLAG_QUIT (1 << 1)
#define VFDM_FLAG_VU (1 << 2)
#define VFDM_FLAG_TRACK (1 << 3)
#define VFDM_FLAG_STATE (1 << 4)
#define VFDM_FLAG_SUSPEND (1 << 5)
#define VFDM_FLAG_RESUME (1 << 6)

/***** Global Variables *****/

/* Text to display and text currently displayed */
static char statustext[12], statusshown[12];

/* Current music playback state */
static enum vfdm_playstate curplaystate;

/* Buffer for indicator values.  To change an indicator, change it here,
 * and then upload via vfd_bmind(ind).  This allows shared usage of
 * indicators.
 */
static unsigned char ind[5];

/* Communicates VU data to VFD thread.
 * When there is no data, vu[0] == -1
 */
static unsigned int vu[2], vu2thread[2];

/* Counter for CPU poll cycles without a Winamp update */
#define NOSCT_FINAL 5
static int nosct;

/* Blocks updates after the display is cleared in preparation for suspend */
static int suspended = 0;

static struct trackdata {
    char name[VFD_SCROLLMAX]; /* Pointer to track name (past track #) */
    int length; /* Number of track name characters to display on VFD */
    int number; /* Track number */
} track2thread, track;

#if 0
/* Parallel queue */
#define PARQLEN 128
#define WM_VFDPAR 0xAFD0
#define WM_VFDPAR_COOKIE 0xE1E75001
static unsigned char parq[PARQLEN];
int parq_head = 0, parq_tail = 0;
#endif

/***** PRIVATE *****/

static void vfdm_setplaystate(enum vfdm_playstate newstate);

void status_upload(int full) {
    if (full) {
        vfd_bmntxt(0, statustext, 12);
        memcpy(statusshown, statustext, 12);
    } else {
        int i;
        for (i = 0; i < 12; i++) {
            if (statusshown[i] != statustext[i]) {
                statusshown[i] = statustext[i];
                vfd_bmsetc(i+1, statustext[i]);
            }
        }
    }
}

void status_puthhmm(unsigned int pos, unsigned long secs) {
    unsigned int hours, mins;

    secs %= 100*60*60; /* 100 hours rolls over */

    hours = secs / (60*60);
    mins = (secs / 60) - (hours * 60);

    if (hours > 0 || mins > 0) {
        statustext[pos+4] = mins % 10 + '0';
        if (hours > 0 || mins > 9) {
            statustext[pos+3] = mins / 10 + '0';
            if (hours > 0) {
                statustext[pos+1] = hours % 10 + '0';
                if (hours > 9) {
                    statustext[pos] = hours / 10 + '0';
                }
            }
        }
    }
}

/***** VFD thread *****/

static void vfdm_thread(void) {
    /* Setup */
    memset(statusshown, 0, 12);
    memset(statustext, ' ', 12);

    vfdm_cb_settimer(1000);

    while (1) {
        /* Wait for us to have something to do */
        unsigned int signalflags;

        signalflags = vfdm_cb_wait();

        if (signalflags == 0) continue;

        /* Mutex is locked. Copy data to avoid race conditions */
        if (signalflags & VFDM_FLAG_VU) {
            vu[0] = vu2thread[0];
            vu[1] = vu2thread[1];
            vu2thread[0] = 0;
            vu2thread[1] = 0;
        }

        if (signalflags & VFDM_FLAG_TRACK) {
            memcpy(&track, &track2thread, sizeof(track));
        }

        vfdm_cb_unlock();

        if (signalflags & VFDM_FLAG_QUIT) break;

        if (signalflags & VFDM_FLAG_SUSPEND) {
            if (!suspended) {
                suspended = 1;
                vfd_bmntxt(0, " ", 1);
                vfd_bmclear();
            }
        }

        if (signalflags & VFDM_FLAG_RESUME) {
            sysmon_pmwake();
            if (suspended) {
                vfd_enterbm();

                vfd_bmind(ind);
                if (nosct < NOSCT_FINAL) {
                    if (track.length > 0) vfd_bmntxt(VFDTXT_LOOP, track.name, track.length);
                }
                vfdm_cb_settimer(10);
                memset(statusshown, ' ', 12);
                memset(statustext, ' ', 12);
                suspended = 0;
            }
        }

        if (!suspended) {
            /* If there is VU meter data send that now */
            if (signalflags & VFDM_FLAG_VU) {
                if (curplaystate != VFDM_PLAYING) {
                    /* Not playing but got VU data -> playing */
                    vfdm_setplaystate(vfdm_cb_getplaystate());
                }

                if (curplaystate == VFDM_PLAYING) {
                    vfd_bmsetvu(vu[0], vu[1]);

                    if (nosct >= NOSCT_FINAL) {
                        signalflags |= VFDM_FLAG_TRACK;
                    } else {
                        nosct = 0;
                    }
                }
            }

            /* Update playing/stopped info */
            if (signalflags & VFDM_FLAG_STATE) {
                vfdm_setplaystate(vfdm_cb_getplaystate());

                if (nosct >= NOSCT_FINAL) {
                    signalflags |= VFDM_FLAG_TRACK;
                } else {
                    nosct = 0;
                }
            }

            /* If track name has changed upload new one */
            if (signalflags & VFDM_FLAG_TRACK) {
                if (nosct >= NOSCT_FINAL) {
                    ind[VFDI_SEC_A] &= ~(VFDI_MIN_B | VFDI_HR_B);
                    vfd_bmind(ind);
                }

                /* Allow track number to be displayed for 5 seconds */
                vfdm_cb_settimer(5000);

                /* Unless nosct is reset during those 5 seconds, the next timer
                 * will cause NOSCT_FINAL state.
                 */
                nosct = NOSCT_FINAL-1;

                vfd_bmntxt(VFDTXT_LOOP, track.name, track.length);
                vfd_bms7dec(track.number);
            }
        } /* !suspended */

        if (signalflags & VFDM_TIMEOUT) {
            vfdm_cb_incrementtimer(1000);
            if (nosct < NOSCT_FINAL) {
                /* Count down for timeout when play stops */
                nosct++;

                if (!suspended) {
                    if (nosct == NOSCT_FINAL) {
                        ind[VFDI_SEC_A] |= VFDI_MIN_B | VFDI_HR_B;
                        vfd_bmind(ind);
                        status_upload(1);
                    } else if (nosct == 2 && curplaystate == VFDM_PLAYING) {
                        /* Probably not playing anymore because there has been
                         * no new data.  This can happen when the last track ends.
                         */
                        vfdm_setplaystate(vfdm_cb_getplaystate());
                    }
                }
            }

            if (!suspended) {
                int i;

                i = sysmon_cpupercent();

                vfd_bms7dec(i);

                if (curplaystate != VFDM_PLAYING) {
                    int m;
                    m = (int)((double)sysmon_memorypercent() * 14.0 / 100 + 0.5);

                    i = (int)((double)i * 14.0 / 100.0 + 0.5);

                    vfd_bmsetvu(i, m);
                }

                {
                    time_t tt = time(NULL);
                    struct tm lt;
                    char *p = &(statustext[0]);
                    static time_t tm_next = 0;

                    status_puthhmm(7, sysmon_getawaketime());

                    if (tt > tm_next) {
#if defined(_MSC_VER)
                        localtime_s(&lt, &tt);
#elif defined(__MINGW32__)
                        memcpy(&lt, localtime(&tt), sizeof(lt));
#else
                        localtime_r(&tt, &lt);
#endif
                        *(p++) = lt.tm_hour / 10 + '0';
                        *(p++) = lt.tm_hour % 10 + '0';
                        *(p++) = ' ';
                        *(p++) = lt.tm_min / 10 + '0';
                        *(p++) = lt.tm_min % 10 + '0';
                        tm_next = tt + (60 - lt.tm_sec);
                    }

                    if (nosct >= NOSCT_FINAL) {
                        status_upload(0);
                    }
                }
            } /* !suspended */
        } /* timeout */

#ifdef ANALOGLOG
        case wait_mintmr:
            if (!suspended) {
                unsigned char adcval[2];
                localtime
                tt = time(NULL);
                adcval[0] = vfd_bmreadadc(0);
                adcval[1] = vfd_bmreadadc(1);
                anbuf_add((unsigned int)tt, adcval, sizeof(adcval));
            } /* !suspended */
#endif
    } /* while(1) */

    /* Cleanup */

#ifdef ANALOGLOG
    anbuf_flush();
#endif

    /* Leave VFD displaying clock */
    vfd_bmclear();
    memset(ind, 0, sizeof(ind));
    ind[VFDI_SEC_A] |= VFDI_SEC_B | VFDI_MIN_B | VFDI_HR_B;
    vfd_bmind(ind);
    vfd_exitbm();

    vfd_setclock(NULL);
    vfd_disconnect();

    sysmon_quit();
}

/***** VFDm API *****/

int vfdm_init(const char *port) {
    /* Initialize variables used for communication with thread */
    memset(ind, 0, sizeof(ind));
    vu2thread[0] = 0;
    vu2thread[1] = 0;

    curplaystate = VFDM_UNKNOWNSTATE;

    nosct = 0;

    /* Connect to and initialize VFD */
    if (vfd_connect(port) != 0) return VFD_FAIL;
    if (vfd_clear() != 0) { vfd_disconnect(); return VFD_FAIL; }
    vfd_enterbm();
    vfd_bmsetscw(1,12);

    sysmon_init();

    vfdm_cb_init(vfdm_thread);

    vfdm_playstatechanged();

    return VFD_OK;
}

void vfdm_close(void) {
    vfdm_cb_lock();
    vfdm_cb_signal(VFDM_FLAG_QUIT);
    vfdm_cb_close();
}

void vfdm_vu(unsigned int l, unsigned int r) {
    vfdm_cb_lock();
    if (vu2thread[0] < l) vu2thread[0] = l;
    if (vu2thread[1] < r) vu2thread[1] = r;
    vfdm_cb_signal(VFDM_FLAG_VU);
}

void vfdm_trackchange(int n, const char *name, unsigned int l) {
    int usablel;

    usablel = (l < VFD_SCROLLMAX) ? l : VFD_SCROLLMAX;

    if (track2thread.number != n ||
        track2thread.length != usablel ||
        memcmp(track2thread.name, name, usablel)) {
        vfdm_cb_lock();
        track2thread.number = n;
        memcpy(track2thread.name, name, usablel);
        track2thread.length = usablel;
        vfdm_cb_signal(VFDM_FLAG_TRACK);
    }
}

void vfdm_playstatechanged(void) {
    vfdm_cb_lock();
    vfdm_cb_signal(VFDM_FLAG_STATE);
}

static void vfdm_setplaystate(enum vfdm_playstate newstate) {
    if (newstate != curplaystate) {
        curplaystate = newstate;
        switch (newstate) {
        case VFDM_PAUSED:
            ind[VFDI_PAUSE_A] |= VFDI_PAUSE_B;
            ind[VFDI_PLAY_A] |= VFDI_PLAY_B;
            ind[VFDI_RIGHT_A] &= ~VFDI_RIGHT_B;
            ind[VFDI_RIGHTVU_A] &= ~VFDI_RIGHTVU_B;
            ind[VFDI_LEFTVU_A] &= ~VFDI_LEFTVU_B;
            vfd_bmsetvu(0, 0);
            break;

        case VFDM_PLAYING:
            ind[VFDI_PAUSE_A] &= ~VFDI_PAUSE_B;
            ind[VFDI_PLAY_A] |= VFDI_PLAY_B;
            ind[VFDI_RIGHT_A] |= VFDI_RIGHT_B;
            ind[VFDI_RIGHTVU_A] |= VFDI_RIGHTVU_B;
            ind[VFDI_LEFTVU_A] |= VFDI_LEFTVU_B;
            break;

        default: /* Stopped or unknown state */
            ind[VFDI_PAUSE_A] &= ~VFDI_PAUSE_B;
            ind[VFDI_PLAY_A] &= ~VFDI_PLAY_B;
            ind[VFDI_RIGHT_A] &= ~VFDI_RIGHT_B;
            ind[VFDI_RIGHTVU_A] &= ~VFDI_RIGHTVU_B;
            ind[VFDI_LEFTVU_A] &= ~VFDI_LEFTVU_B;
            vfd_bmsetvu(0, 0);
            break;
        }

        if (!suspended) vfd_bmind(ind);
    }
}

void vfdm_pmsuspend(void) {
    vfdm_cb_lock();
    vfdm_cb_signal(VFDM_FLAG_SUSPEND);
}

void vfdm_pmwake(void) {
    vfdm_cb_lock();
    vfdm_cb_signal(VFDM_FLAG_RESUME);
}
