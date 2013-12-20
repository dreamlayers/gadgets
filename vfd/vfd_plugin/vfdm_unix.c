/* POSIX-specific code for the VFD display music player plugin. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "vfdm.h"

static struct timespec timer;
static int timerset;
static unsigned int flags;

static pthread_cond_t cond;
static pthread_mutex_t mtx;
static pthread_t thr;

static void (*threadfunc)(void);

static void *thread(void *param){
    (void)param;
    threadfunc();
    return NULL;
}

int vfdm_cb_init(void (*threadmain)(void)) {
    int res;

    timerset = 0;
    flags = 0;

    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&cond, NULL);

    threadfunc = threadmain;
    res = pthread_create(&thr, NULL, thread, NULL);
    if (res != 0) return -1;

    return 0;
}

void vfdm_cb_close(void) {
    pthread_join(thr, NULL);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mtx);
}

static void timespec_add_ms(struct timespec *ts, unsigned int milliseconds) {
    ts->tv_sec += milliseconds / 1000;
    ts->tv_nsec += (milliseconds % 1000) * 1000000;

    if (ts->tv_nsec > 1000000000) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000;
    }
}

void vfdm_cb_settimer(unsigned int milliseconds) {
    timerset = 0;

    clock_gettime(CLOCK_REALTIME, &timer);
    timespec_add_ms(&timer, milliseconds);

    timerset = 1;
}

void vfdm_cb_incrementtimer(unsigned int milliseconds) {
    timespec_add_ms(&timer, milliseconds);
    timerset = 1;
}

unsigned int vfdm_cb_wait(void) {
    unsigned int flagscopy;
    struct timespec timenow;

    clock_gettime(CLOCK_REALTIME, &timenow);
    if (timenow.tv_sec >= timer.tv_sec ||
        (timenow.tv_sec == timer.tv_sec &&
         timenow.tv_nsec >= timer.tv_nsec)) {
        timerset = 0;
        return VFDM_TIMEOUT;
    }

    // Mutex is unlocked only when we wait or process received events.
    pthread_mutex_lock(&mtx);

    // Here should be race-condition prevention in real code.

    while(1)
    {
        if (flags)
        {
            flagscopy = flags;
            flags = 0;

            return flagscopy;
        }
        else
        {
            int res;
            // Mutex is locked. It is unlocked while we are waiting.
            if (timerset) {
                res = pthread_cond_timedwait(&cond, &mtx, &timer);
                if (res == ETIMEDOUT) {
                    timerset = 0;
                    flags |= VFDM_TIMEOUT;
                }
            } else {
                pthread_cond_wait(&cond, &mtx);
            }
        }
    }
}

void vfdm_cb_signal(unsigned int flagstoset) {
    flags |= flagstoset;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mtx);
}

void vfdm_cb_lock(void) {
    pthread_mutex_lock(&mtx);
}

void vfdm_cb_unlock(void) {
    pthread_mutex_unlock(&mtx);
}
