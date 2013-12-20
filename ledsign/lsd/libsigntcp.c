/* Library for accessing the LED sign via the lsd daemon. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <string.h>
#define IN_SDCLIENT
#include "sdclient.h"
#include "libsigntcp.h"

/* Port to listen on */
const int tcpport = 9876;

/* Protocol version */
const char protver[] = "LEDsign_1.0\r\n";

/* Sign commands */
int sc_nop() {
  return sc_xsimplec('N', 0, 0, 0);
}

int sc_clear(unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xsimplec('C', flags, transtm, gntm);
}

int sc_full(unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xsimplec('F', flags, transtm, gntm);
}

int sc_hnprint(char *msg, unsigned l, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xdatac('P', msg, l, flags | SFLAG_ASCIIZ, transtm, gntm);
}

int sc_snprint(char *msg, unsigned l, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xdatac('R', msg, l, flags | SFLAG_ASCIIZ, transtm, gntm);
}

int sc_gnprint(char *msg, unsigned l, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xdatac('G', msg, l, flags | SFLAG_ASCIIZ, transtm, gntm);
}

int sc_hprint(char *msg, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xdatac('P', msg, strlen(msg), flags | SFLAG_ASCIIZ, transtm, gntm);
}

int sc_sprint(char *msg, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xdatac('R', msg, strlen(msg), flags | SFLAG_ASCIIZ, transtm, gntm);
}

int sc_gprint(char *msg, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xdatac('G', msg, strlen(msg), flags | SFLAG_ASCIIZ, transtm, gntm);
}

int sc_dl(char *buf) {
  return -1;
}

int sc_ul(char *buf, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  return sc_xdatac('U', buf, 63, flags, transtm, gntm);
}

#if 0
int sc_status(char *FIXME) {
  return -1;
}
#endif

/* SCMD_VIEW - Unimplemented, only for interactive testing of server */

#ifdef TESTING
int main() {
  sc_open();

  sc_sprint("test\rme\ntoo",0,0,0);

  sc_close();

  return 0;
}
#endif
