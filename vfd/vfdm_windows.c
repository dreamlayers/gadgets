#define __W32API_USE_DLLIMPORT__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "vfdm.h"

static ULONGLONG timertime;
static int timerset;

static HANDLE VFDtodo;
static HANDLE VFDThreadH;

static void (*threadfunc)(void);

static DWORD WINAPI thread(LPVOID lpParameter) {
    threadfunc();
    return 0;
}

int vfdm_cb_init(void (*threadmain)(void)) {
    DWORD ThreadId;

    VFDtodo = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (VFDtodo == NULL) return -1;

    threadfunc = threadmain;
    VFDThreadH = CreateThread(NULL, 10000, &thread, NULL, 0, &ThreadId);
    if (VFDThreadH == NULL) return -1;

    return 0;
}

void vfdm_cb_close(void) {
    WaitForSingleObject(VFDThreadH, INFINITE);
    CloseHandle(VFDThreadH);
    CloseHandle(VFDtodo);
}

void vfdm_cb_settimer(unsigned int milliseconds) {
    timertime = GetTickCount64() + (ULONGLONG)milliseconds;
    timerset = 1;
}

void vfdm_cb_incrementtimer(unsigned int milliseconds) {
    timertime += (ULONGLONG)milliseconds;
    timerset = 1;
}

enum vfdm_signalstate vfdm_cb_wait(void) {
    DWORD res;
    ULONGLONG curtime;

    curtime = GetTickCount64();

    /* Ensure timeouts aren't blocked by signals */
    if (timerset && curtime >= timertime) {
        timerset = 0;
        return VFDM_TIMEOUT;
    }

    res = WaitForSingleObject(VFDtodo,
                              timerset ? ((DWORD)(timertime - curtime)) : INFINITE);
    if (res == WAIT_TIMEOUT) {
        timerset = 0;
        return VFDM_TIMEOUT;
    } else if (res == WAIT_OBJECT_0) {
        return VFDM_SIGNALLED;
    } else {
        return VFDM_SIGNALERR;
    }
}

void vfdm_cb_signal(void) {
    SetEvent(VFDtodo);
}