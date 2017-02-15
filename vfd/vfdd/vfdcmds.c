/* Glue between the socket/daemon code and the VFD direct access library. */
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
#include <sys/time.h>
#include <errno.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LPSOCKADDR (struct sockaddr *)
#define SOCKET int
#endif

#include "vfd.h"
#include "signd.h"
//#include "serio.h" /* FIXME */

/*
 * Constant data
 */

/* Protocol version */
const char cmd_protocolid[] = "VFD_1.0\r\n";

static int sc_print(scmdblk *scb) {
    unsigned int flags = 0;

    if (scb->flags & SFLAG_LOOP) flags |= VFDTXT_LOOP;
    if (scb->flags & SFLAG_APPEND) flags |= VFDTXT_APPEND;

    if (scb->data != NULL && ((chainhdr *)(scb->data))->size > 0) {
        return vfd_bmntxt(flags, (char *)(scb->data + sizeof(chainhdr)),
                          ((chainhdr *)(scb->data))->size);
    } else {
        return vfd_bmntxt(flags, NULL, 0);
    }
}

static int sc_clear(scmdblk *scb) {
    vfd_bmclear(); return 0;
}

/*
 * Data about commands for use by generic daemon code.
 */

/* Letters for sign commands */
const char *cmd_commands = "HNCP";

/* Data about sign commands */
signcmd cmd_cdata[] = {
    { NULL, NULL, sc_r_help,
      0,
      "Help" },
    { NULL, NULL, NULL,
      SFLAG_QUEUE,
      "No operation" },
    { NULL, sc_clear, NULL,
      SFLAG_QUEUE | SFLAG_TRANS | SFLAG_GNTEE,
      "Clear sign" },
    { sc_d_msg, sc_print, NULL,
      SFLAG_ASCIIZ | SFLAG_APPEND | SFLAG_LOOP | SFLAG_QUEUE |
      SFLAG_TRANS | SFLAG_GNTEE | SFLAG_ONCE,
      "Print message" },
};

/*
 * Responder functions: take data from scmdblk and respond with result of execution
 */


/*
 * Functions called directly and not related to commands.
 */

int cmd_clear(void) {
    vfd_bmclear(); return 0;
}

int cmd_init(const char *device) {
  static const char *defdev =
#if defined(WIN32)
    "COM1";
#elif defined(__CYGWIN__)
    "/dev/ttyS0";
#elif defined(__linux)
    "/dev/ttyS0";
#else
    "/dev/cu.serial1";
#endif

  if (vfd_connect((device != NULL) ? device : defdev) != 0) {
    return -1;
  }

  return vfd_enterbm();
}

void cmd_cleanup(void) {
    vfd_exitbm();
    vfd_disconnect();
}

int cmd_need_keepalive(void) {
    return vfd_need_keepalive();
}

int cmd_call_keepalive(void) {
    return vfd_call_keepalive();
}
