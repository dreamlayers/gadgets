/* Linux system monitor for the VFD display music player plugin. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "sysmon.h"

/*** CPU ***/

static unsigned long used, free;

static int readprocstat(unsigned long *u, unsigned long *f) {
    FILE *procstat;
    char buf[4];
    unsigned long user, nice, system;
    int res;
    procstat = fopen("/proc/stat", "r");
    if (procstat == NULL) return -1;
    res = fscanf(procstat, "%3s %lu %lu %lu %lu",
                 buf, &user, &nice, &system, f);
    fclose(procstat);
    if (res != 5) return -1;
    *u = user + nice + system;
    return 0;
}

int sysmon_cpupercent(void) {
    unsigned long newused, newfree;
    unsigned long useddelta, totaldelta;
    if (readprocstat(&newused, &newfree) != 0) return 0;
    useddelta = newused - used;
    totaldelta = useddelta + newfree - free;
    used = newused;
    free = newfree;
    if (totaldelta == 0) return 0;
    return (int)((double)useddelta * 100.0 / (double)totaldelta + 0.5);
}

/*** Memory ***/

int sysmon_memorypercent(void) {
    char buf[20];
    FILE *f;
    unsigned long total = 0, active = 0;
    int gottotal = 0, gotactive = 0;

    f = fopen("/proc/meminfo", "r");
    if (f == NULL) return 0;
    while (1) {
        unsigned long val;
        if (fscanf(f, "%19[^:]: %lu kB\n", buf, &val) != 2) break;
        if (!gottotal && !strcmp(buf, "MemTotal")) {
            total = val;
            gottotal = 1;
            if (gotactive) break;
        } else if (!gotactive && !strcmp(buf, "Active")) {
            active = val;
            gotactive = 1;
            if (gottotal) break;
        }
    }
    fclose(f);
    if (gottotal && gotactive && total > 0) {
        return (int)((double)active * 100.0 / (double)total + 0.5);
    } else {
        return 0;
    }
}

/*** Wake time ***/

static time_t lastwaketime = 0;

static time_t getmtime(const char *fn) {
    struct stat st;
    if (stat(fn, &st) == 0) {
        return st.st_mtime;
    } else {
        return 0;
    }
}

static time_t getlastwaketime(void) {
    time_t pmlogt, bootlogt;
    pmlogt = getmtime("/var/log/pm-suspend.log");
    bootlogt = getmtime("/var/log/boot.log");
    return (pmlogt > bootlogt) ? pmlogt : bootlogt;
}

void sysmon_pmwake(void) {
    lastwaketime = time(NULL);
}

unsigned long sysmon_getawaketime(void) {
    if (lastwaketime != 0) {
        return time(NULL) - lastwaketime;
    } else {
        return 0;
    }
}

/*** Misc ***/

int sysmon_init(void) {
    used = 0; free = 0;
    lastwaketime = getlastwaketime();
    pm_upower_init();
    return 0;
}

void sysmon_quit(void) {
}

#ifdef TEST_SYSMON
int main(void) {
    return 0;
}
#endif
