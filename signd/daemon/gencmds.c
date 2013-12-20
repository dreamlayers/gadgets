/* Generic commands for TCP server for sign gadgets. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LPSOCKADDR (struct sockaddr *)
#define SOCKET int
#endif

#include "signd.h"

/* Free chain such as scb->data */
void cmd_freedata(void *p) {
  chainhdr *cp1, *cp2;

  cp1 = (chainhdr *)p;
  while (cp1) {
    cp2 = cp1->next;
    free(cp1);
    cp1 = cp2;
  }
}

int sc_read_ulong(SOCKET sock, unsigned long *out) {
  unsigned char c;

  *out = 0;

  while (1) {
    if (recv(sock, (char *)&c, 1, 0) != 1) break;

    if (c == ';') return 0;
    if (c < '0' || c > '9') break;

    if (*out > (ULONG_MAX / (unsigned long)10)) break;
    *out *= 10;

    c -= '0';
    if (*out > (ULONG_MAX - (unsigned long)c)) break;
    *out += c;
  }

  return -1;
}

static int sc_p_trans(scmdblk *scmd, SOCKET sock) {
  return sc_read_ulong(sock, &(scmd->transtime));
}

static int sc_p_gntee(scmdblk *scmd, SOCKET sock) {
  return sc_read_ulong(sock, &(scmd->gnteetime));
}

int sc_d_msg(scmdblk *cmd, SOCKET sock) {
  char c;
  /* Pointer to pointer to current block */
  chainhdr **ptc = (chainhdr **)&(cmd->data);
  int remain = 0;                 /* Remaining bytes in current block */
  chainhdr *curb = NULL;          /* Pointer to current block */
  char *p = NULL;                 /* Pointer to where the next byte goes */

  *ptc = NULL;

  while ((recv(sock, &c, 1, 0) == 1) &&
         ((cmd->flags & SFLAG_ASCIIZ) ?
          (c != 0) :
          (c != '\r' && c != '\n'))) {
    if (remain == 0) {
      if (curb != NULL) {
        ptc = &(curb->next);
        curb->size = CHAINBLK - sizeof(chainhdr);
      } /* Else already set to cmd->data */
      curb = (chainhdr *)malloc(CHAINBLK);
      if (curb == NULL) return -1;
      *ptc = curb;
      curb->next = NULL;

      remain = CHAINBLK - sizeof(chainhdr);
      p = ((char *)curb) + sizeof(chainhdr);
    }
    *p = c;
    p++;
    remain--;
  }

  if (curb != NULL) {
    if (remain != 0) {
      *ptc = (chainhdr *)realloc(curb, CHAINBLK - remain);
      if (*ptc == NULL) return -1;
    }
    (*ptc)->size = CHAINBLK - sizeof(chainhdr) - remain;
  }

  return 0;
}

int sc_r_help(scmdblk *scb, SOCKET sock) {
    int i, nRet;
    char buf[128];
    const char *cp;

    (void)scb;

    nRet = (int)send(sock, "\r\nFlags:", 8, 0);
    if (nRet == SOCKET_ERROR) return -1;

    for (i = 0; cmd_flags[i] != 0; i++) {
        buf[0] = '\r';
        buf[1] = '\n';
        buf[2] = cmd_flags[i];
        buf[3] = ' ';
        nRet = (int)send(sock, buf, 4, 0);
        if (nRet == SOCKET_ERROR) return -1;

        if (cmd_fdata[i].helptext != NULL) {
            nRet = (int)send(sock, cmd_fdata[i].helptext,
                             strlen(cmd_fdata[i].helptext), 0);
            if (nRet == SOCKET_ERROR) return -1;
        }
    }
    if (nRet == SOCKET_ERROR) return -1;

    nRet = (int)send(sock, "\r\n\r\nCommands:", 13, 0);
    if (nRet == SOCKET_ERROR) return -1;

    for (i = 0; cmd_commands[i] != 0; i++) {
        unsigned int cm;
        char *bufp;

        buf[0] = '\r';
        buf[1] = '\n';
        buf[2] = cmd_commands[i];
        buf[3] = ' ';
        buf[4] = '[';
        bufp = &buf[5];

        for (cp = &cmd_flags[0], cm = 1;
             *cp != 0;
             cp++, cm <<= 1) {
            if (cmd_cdata[i].flags & cm) {
                *(bufp++) = *cp;
            }
        }

        *(bufp++) = ']';
        *(bufp++) = ' ';

        nRet = (int)send(sock, buf, bufp-buf, 0);
        if (nRet == SOCKET_ERROR) return -1;

        if (cmd_cdata[i].helptext != NULL) {
            nRet = (int)send(sock, cmd_cdata[i].helptext,
                             strlen(cmd_cdata[i].helptext), 0);
            if (nRet == SOCKET_ERROR) return -1;
        }
    }
    nRet = (int)send(sock, "\r\n", 2, 0);
    if (nRet == SOCKET_ERROR) return -1;
    return 0;
}

/*
 * Standard flags that can be used for any command
 */

/* Letters for command modifier flags */
const char *cmd_flags = "ALZQTGO";

/* Data about flags */
signflag cmd_fdata[] = {
    { NULL, "Append" },
    { NULL, "Loop" },
    { NULL, "Null terminated string" },
    { NULL, "Queue only" },
    { sc_p_trans, "Transient time" },
    { sc_p_gntee, "Guaranteed time" },
    { NULL, "Once only" }
};
