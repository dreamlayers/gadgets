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

int sc_help(scmdblk *scb, SOCKET sock) {
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
