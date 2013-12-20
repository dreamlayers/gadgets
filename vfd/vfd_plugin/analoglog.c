/* ADC logger for the VFD display. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <string.h>
#include <stdio.h>

#define LOGBUFSIZE 1024
#define LOGFILE "C:\\Users\\Boris\\Documents\\vfd.log"

unsigned char buf[LOGBUFSIZE];
static unsigned int bufptr = 0;

static void bufput(unsigned const char *data, unsigned int l) {
    memcpy(&buf[bufptr], data, l);
    bufptr += l;
}

void anbuf_flush(void) {
    if (bufptr > 0) {
        unsigned int l = bufptr;
#ifdef _MSC_VER
        FILE *f;
        fopen_s(&f, LOGFILE, "ab");
#else
        FILE *f = fopen(LOGFILE, "ab");
#endif
        bufptr = 0;
        if (f == NULL) return;
        fwrite(buf, l, 1, f);
        fclose(f);
    }
}

void anbuf_add(unsigned int cts, unsigned const char *data, unsigned int l) {
    if (bufptr + sizeof(cts) + l >= LOGBUFSIZE) anbuf_flush();

    bufput((unsigned char *)&cts, sizeof(cts));
    bufput(data, l);
}

