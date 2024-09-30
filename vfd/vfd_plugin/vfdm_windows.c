/* Windows-specific code for the VFD display music player plugin. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#define __W32API_USE_DLLIMPORT__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "vfdm.h"

#ifdef __MINGW32__
ULONGLONG WINAPI __declspec(dllimport) GetTickCount64(void);
#endif

static ULONGLONG timertime;
static int timerset;
static unsigned int flags;

static HANDLE VFDtodo;
static HANDLE VFDThreadH;
static HANDLE mtx;

static void (*threadfunc)(void);

static DWORD WINAPI thread(LPVOID lpParameter) {
    threadfunc();
    return 0;
}

int vfdm_cb_init(void (*threadmain)(void)) {
    DWORD ThreadId;

    timerset = 0;
    flags = 0;

    VFDtodo = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (VFDtodo == NULL) return -1;

    mtx = CreateMutex(NULL, FALSE, NULL);
    if (mtx == NULL) return -1;

    threadfunc = threadmain;
    VFDThreadH = CreateThread(NULL, 10000, &thread, NULL, 0, &ThreadId);
    if (VFDThreadH == NULL) return -1;

    return 0;
}

void vfdm_cb_close(void) {
    WaitForSingleObject(VFDThreadH, INFINITE);
    CloseHandle(VFDThreadH);
    CloseHandle(VFDtodo);
    CloseHandle(mtx);
}

void vfdm_cb_settimer(unsigned int milliseconds) {
    timertime = GetTickCount64() + (ULONGLONG)milliseconds;
    timerset = 1;
}

void vfdm_cb_incrementtimer(unsigned int milliseconds) {
    timertime += (ULONGLONG)milliseconds;
    timerset = 1;
}

unsigned int vfdm_cb_wait(void) {
    DWORD res;
    ULONGLONG curtime;
    unsigned int flagscopy = 0;

    while (1) {
        /* Ensure timeouts aren't blocked by signals */
        curtime = GetTickCount64();
        if (timerset && curtime >= timertime) {
            flagscopy |= VFDM_TIMEOUT;
            timerset = 0;
        } else {
            res = WaitForSingleObject(VFDtodo,
                                      timerset ? ((DWORD)(timertime - curtime)) : INFINITE);
            if (timerset && res == WAIT_TIMEOUT) {
                flagscopy |= VFDM_TIMEOUT;
                timerset = 0;
            } else if (res != WAIT_OBJECT_0) {
                return 0;
            }
        }

        WaitForSingleObject(mtx, INFINITE);
        flagscopy |= flags;
        flags = 0;
        if (flagscopy != 0) {
            break;
        } else {
            ReleaseMutex(mtx);
        }
    }
    return flagscopy;
}

void vfdm_cb_signal(unsigned int flagstoset) {
    flags |= flagstoset;
    ReleaseMutex(mtx);
    /* Setting an event that has been already set has no effect */
    SetEvent(VFDtodo);
}

void vfdm_cb_lock(void) {
    WaitForSingleObject(mtx, INFINITE);
}

void vfdm_cb_unlock(void) {
    ReleaseMutex(mtx);
}
