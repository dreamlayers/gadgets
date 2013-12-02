#include <stdio.h>
#include <string.h>
#ifdef WIN32
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LPSOCKADDR (struct sockaddr *)
#define SOCKET int
#endif
#define IN_SDCLIENT
#include "sdclient.h"

#define NUMRESP 4
static const char *resp[NUMRESP] = {
    SPROTO_OK, SPROTO_QUEUED, SPROTO_TIMEOUT, SPROTO_FAILED
};

/* Letters for command modifier flags */
static const char signflagl[] = SFLAG_CHARACTERS;

/* Static variables */
static SOCKET sc_xsock = INVALID_SOCKET;
static int    sc_keepopen = 0;

/* Actually open connection to sign */
static int sc_xopen(void) {
    int nRet;
#ifdef WIN32
    WORD version = MAKEWORD(1,1);
    WSADATA wsaData;
    SOCKET theSocket;
    LPHOSTENT lpHostEntry;
    SOCKADDR_IN saServer;

    /* Start up Winsock */
    WSAStartup(version, &wsaData);
#else
    struct hostent *lpHostEntry;
    int theSocket;
    struct sockaddr_in saServer;
#endif

    /* Store information about the server */
    lpHostEntry = gethostbyname("localhost");
    if (lpHostEntry == NULL) {
      printf("Error at gethostbyname()");
#ifdef WIN32
      WSACleanup();
#endif
      return -1;
    }

    /* Create the socket */
    theSocket = socket(AF_INET,            /* Go over TCP/IP */
                       SOCK_STREAM,        /* Socket type */
                       IPPROTO_TCP);    /* Protocol */
    if (theSocket == INVALID_SOCKET) {
        printf("Error at socket()");
#ifdef WIN32
      WSACleanup();
#endif
        return -1;
    }


    /* Use SOCKADDR_IN to fill in address information */
    saServer.sin_family = AF_INET;
#ifdef WIN32
    saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
#else
    saServer.sin_addr = *((struct in_addr *)*lpHostEntry->h_addr_list);
#endif
    saServer.sin_port = htons(tcpport);

    /* Connect to the server */
    nRet = connect(theSocket,
               (struct sockaddr *)&saServer,        // Server address
               sizeof(struct sockaddr));    // Length of address structure
        if (nRet == SOCKET_ERROR) {
       /* FIXME:  closesocket ? */
       printf("Error at connect()");
#ifdef WIN32
       WSACleanup();
#endif
       return -1;
    }
    sc_xsock = theSocket;

    return 0;
}

/* Actually close connection to sign */
static void sc_xclose(void) {
  if (!sc_keepopen && sc_xsock != INVALID_SOCKET) {
#ifdef WIN32
    closesocket(sc_xsock);
    WSACleanup();
#else
    close(sc_xsock);
#endif
    sc_xsock = INVALID_SOCKET;
  }
}

int sc_open(void) {
  int res;

  res = sc_xopen();
  if (res == 0) {
    sc_keepopen = 1;
  }
  return res;
}

void sc_close(void) {
  sc_keepopen = 0;
  sc_xclose();
}

static int sc_xsendc(char c) {
  return  (send(sc_xsock, &c, 1, 0) == 1) ? 0 : -1;
}

static int sc_xcmdulpar(unsigned long transtm) {
  char bfr[12];
  int l;

#ifdef _MSC_VER
  l = sprintf_s(&(bfr[0]), sizeof(bfr), "%lu;", transtm);
#else
  l = snprintf(&(bfr[0]), sizeof(bfr), "%lu;", transtm);
#endif

  return  (send(sc_xsock, bfr, l, 0) == l) ? 0 : -1;
}

static int sc_xbegincmd(char c, unsigned long flags, unsigned long transtm, unsigned long gntm) {
  int i;

  if (!sc_keepopen) sc_xopen();

  if (sc_xsendc(c) != 0) {
    if (sc_keepopen) {
      sc_xclose();
      if (sc_xopen() != 0) return -1;
      if (sc_xsendc(c) != 0) return -1;
    } else {
      return -1;
    }
  }

  if (flags != 0) {
    if (sc_xsendc('/') != 0) return -1;

    for (i = 0; i < sizeof(signflagl); i++) {
      if (flags & 1) {
        if (sc_xsendc(signflagl[i]) != 0) return -1;
        switch(signflagl[i]) {
        case 'T': if (sc_xcmdulpar(transtm) != 0) return -1;
                  break;
        case 'G': if (sc_xcmdulpar(gntm) != 0) return -1;
                  break;
        }
      }
      flags >>= 1;
    }
  }
  return 0;
}

static int sc_xfinishcmd(char c, unsigned long flags) {
  char inpt;
  int ri[NUMRESP], i;

  for (i = 0; i < NUMRESP; i++) {
    ri[i] = 0;
  }

  /* Perhaps FIXME */
  while (recv(sc_xsock, &inpt, 1, 0) == 1) {
    for (i = 0; i < NUMRESP; i++) {
      if (resp[i][ri[i]] == inpt) {
        ri[i]++;
        if (resp[i][ri[i]] == 0) goto gotresp;
      } else {
        ri[i] = 0;
      }
    }
  }
  return -1;

gotresp:
  if ( (flags & SFLAG_QUEUE) ? (i == 1) : (i == 0) ) {
    return 0;
  } else {
    return -1;
  }

  #if 0
  int haso = 0;

  while (recv(sc_xsock, &c, 1, 0) == 1) {
    switch(c) {
    case 'O': haso = 1; break;
    case 'K': if (haso) { return 0; } else haso = 0; break;
    default:  haso = 0;
    }
  }
  #endif
}

int sc_xdatac(char c, char *b, unsigned l, unsigned long flags,
              unsigned long transtm, unsigned long gntm) {
  int res;

  res = sc_xbegincmd(c, flags, transtm, gntm);
  if (res != 0) goto sc_xdcfail;

  res = sc_xsendc(':');
  if (res != 0) goto sc_xdcfail;

  res = -1;
  if (send(sc_xsock, (char *)b, l, 0) != l) goto sc_xdcfail;

  if (flags & SFLAG_ASCIIZ) {
    res = sc_xsendc(0);
    if (res != 0) goto sc_xdcfail;
  }

  res = sc_xfinishcmd(c, flags);

sc_xdcfail:
  if (!sc_keepopen) sc_xclose();
  return res;
}

int sc_xsimplec(char c, unsigned long flags,
                unsigned long transtm, unsigned long gntm) {
  int res;

  res = sc_xbegincmd(c, flags, transtm, gntm);
  if (res != 0) goto sc_xscfail;

  res = sc_xsendc('\r');
  if (res != 0) goto sc_xscfail;

  return sc_xfinishcmd(c, flags);

sc_xscfail:
  if (!sc_keepopen) sc_xclose();
  return -1;
}
