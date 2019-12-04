/* Glue between the daemon code and the LED sign direct access library. */
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

#include "ledsign.h"
#include "signd.h"

/* size of allocation block for scmd->data */
#define CHAINBLK 10000

#include "6x7font.h"

/*
 * Constant data
 */

/* Protocol version */
const char cmd_protocolid[] = "LEDsign_1.0\r\n";

/*
 * Parameter functions: read parameters and put them in the scmdblk
 */

/*
 * Command input functions: store input of commands in the scmdblk
 */

static int recvfixed(SOCKET s, char *b, size_t l) {
  char *p = b;
  size_t remain = l;
  size_t got;

  while (remain) {
    got = recv(s, p, remain, 0);
    if (got <= 0) return -1;
    remain -= got;
    p += got;
  }

  return 0;
}

static int pscmd_ul(scmdblk *cmd, SOCKET sock) {
  cmd->data = (unsigned char *)malloc(63 + sizeof(chainhdr));
  if (cmd->data == NULL) return -1;

  ((chainhdr *)(cmd->data))->next = NULL;
  ((chainhdr *)(cmd->data))->size = 63;

  return recvfixed(sock, (char *)(cmd->data) + sizeof(chainhdr), 63);
}

/*
 * Executor functions: perform the operation in scmdblk and put result in scmdblk
 */

static int xsc_printnull(scmdblk *scb) {
  if (!(scb->flags & SFLAG_APPEND)) {
    return sign_clear();
  } else {
    return 0;
  }
}

static void xsc_nextchar(chainhdr **bp, char **p, int *remain) {
  (*remain)--;
  if (!(*remain)) {
    (*bp) = (*bp)->next;
    if ((*bp) != NULL) {
      *p = (char *)(*bp) + sizeof(chainhdr);
      *remain = (*bp)->size;
    } else {
      *p = NULL;
    }
  } else {
    (*p)++;
  }
}

static int xsc_bfrchars(char *b, int l, chainhdr *loopstart, chainhdr **bp, char **p, int *remain) {
  int chunk;
  int got = 0;
  int ended = 0;

  while (got < l) {
    if (*remain == 0) {
      *bp = (*bp)->next;
      if ((*bp) == NULL) {
        if (loopstart == NULL) {
          return -got;
        } else {
          (*bp) = loopstart;
          ended = 1;
        }
        *remain = (*bp)->size;
        *p = (char *)(*bp) + sizeof(chainhdr);
      }
    }

    chunk = l - got;
    if (chunk > *remain) chunk = *remain;
    memcpy(&(b[got]), *p, chunk);
    got += chunk;
    *remain -= chunk;
    *p += chunk;
  }

  if (!ended) {
    return got;
  } else {
    return -got;
  }
}

static int xsc_bfrline(char *b, int l, chainhdr **bp, char **p, int *remain) {
  int i;
  chainhdr *lbp;
  char *lp;
  int lremain;

  for (i = 0; i < l; i++) {
    if (*p == NULL) return i;
    b[i] = **p;
    lbp = *bp; lp = *p; lremain = *remain;
    xsc_nextchar(bp, p, remain);
    if (b[i] == '\r' || b[i] == '\n') {
      *bp = lbp; *p = lp; *remain = lremain;
      return i;
    }
  }

  return l;
}

static int sc_sprint (scmdblk *scb) {
  int res, i, remain, qnow, lmt = 0;
  int ntfy = 1;
  char b[72];
  chainhdr *bp;
  char *p;
  char c;
  int inscrl = 0;

  bp = (chainhdr *)scb->data;
  if (bp == NULL || bp->size == 0) {
    /* Nothing to print */
    return xsc_printnull(scb);
  } else {
    p = (char *)bp + sizeof(chainhdr);
    remain = bp->size;

    /* If not append upload the first bit */
    if (!(scb->flags & SFLAG_APPEND)) {
      /* Begin by clearing and uploading */
      res = sign_clear();
      if (res != 0) return res;

      lmt = 72 / font1.width;

      i = xsc_bfrline(&(b[0]), lmt, &bp, &p, &remain);

      if (scb->flags & SFLAG_LOOP) {
        while (i < lmt && p == NULL) {
          bp = (chainhdr *)scb->data;
          p = (char *)bp + sizeof(chainhdr);
          remain = bp->size;
          i += xsc_bfrline(&(b[i]), lmt - i, &bp, &p, &remain);
        }
      }

      sign_ul_nstr(&(b[0]), i, &font1, 0);

      if (cmd_cb_pollquit()) return 0;
    }

    /* Loop for message looping */
    while (1) {
      /* Loop for going through message */
      while (1) {
        /* Get character */
        if (p == NULL) break;
        c = *p;

        if (c == '\r' || c == '\n') {
          /* Reset pointer for character after */
          xsc_nextchar(&bp, &p, &remain);
          if (p == NULL) break;

          /* Handle CRLF */
          if ((*p == '\r' || *p == '\n') && *p != c) {
            xsc_nextchar(&bp, &p, &remain);
            if (p == NULL) break;
          }

          /* Starting new line, scroll it up */
          i = xsc_bfrline(&(b[0]), lmt, &bp, &p, &remain);
          res = sign_scru_nstr(&(b[0]), i, &font1, 0);
          if (res != 0) return -1;
          if (cmd_cb_pollquit()) return 0;
        } else {
          /* Continue horizontal scrolling */
          if (!inscrl) {
            res = sign_scrl_start();
            if (res != 0) return res;
          }

          /* Reset pointer for character after */
          xsc_nextchar(&bp, &p, &remain);

          /* Horizontally scroll character */
          qnow = cmd_cb_pollquit();
          if (qnow) {
            inscrl = 0;
          } else if (p == NULL) {
            if (scb->flags & SFLAG_LOOP) {
              inscrl = (*(scb->data + sizeof(chainhdr)) != '\r' &&
                       *(scb->data + sizeof(chainhdr)) != '\n');
            } else {
              inscrl = 0;
            }
          } else {
            inscrl = (*p != '\r' && *p != '\n');
          }
          res = sign_scrl_char(c, &font1, 0, inscrl);
          if (res != 0 || qnow) return res;
        }
      }

      if (scb->flags & SFLAG_LOOP) {
        /* Notify after one completion */
        if (ntfy) {
          cmd_cb_notify(scb);
          ntfy = 0;
        }

        /* Reset to start */
        bp = (chainhdr *)scb->data;
        p = (char *)bp + sizeof(chainhdr);
        remain = bp->size;
      } else {
        /* No looping */
        break;
      }
    }
  }

  return 0;
}


/* Print using hardware */
static int sc_hprint(scmdblk *scb) {
    char bfr[17];
    int l, res, remain;
    int ntfy = 1;
    int ending = 0;
    chainhdr *bp;
    char *p;
    unsigned int flags = 0;

    if (scb->data == NULL || ((chainhdr *)(scb->data))->size == 0) {
        /* Nothing to print */
        return xsc_printnull(scb);
    } else if (((chainhdr *)(scb->data))->size <= 17) {
        /* Do it with 1 command */
        l = ((chainhdr *)(scb->data))->size;

        if (scb->flags & SFLAG_LOOP) flags |= SIGN_LOOP;
        if (scb->flags & SFLAG_APPEND) flags |= SIGN_APPEND;

        return sign_nhwprint((char *)(scb->data + sizeof(chainhdr)), l, flags);
    } else {
        /* Do it with multiple commands, keep issuing */
        if (scb->flags & SFLAG_APPEND) flags |= SIGN_APPEND;

        bp = (chainhdr *)(scb->data);
        remain = bp->size;
        p = (char *)bp + sizeof(chainhdr);

        do {
            l = xsc_bfrchars(&(bfr[0]), 17, (chainhdr *)((scb->flags & SFLAG_LOOP) ? scb->data : NULL),
                             &bp, &p, &remain);

            if (l < 0) {
                ending = 1;
                l = -l;
            }
            if (l != 0) {
                fwrite(&bfr[0], l, 1, stderr);
                res = sign_nhwprint(&(bfr[0]), l, flags);
            } else
                ending = 1;
            if (res != 0) return res;
            if (cmd_cb_pollquit()) return 0;

            if (ending && (scb->flags & SFLAG_LOOP)) {
                if (ntfy) {
                    cmd_cb_notify(scb);

                    ntfy = 0;
                }
                ending = 0;
            }

            flags |= SIGN_APPEND;
        } while (!ending);

        return 0;
    }
}

static int sc_clear(scmdblk *scb) {
    return sign_clear();
}

int cmd_clear(void) {
    return sign_clear();
}

static int sc_full(scmdblk *scb) {
    return sign_full();
}

static int sc_dl(scmdblk *scb) {
    int res;
    scb->response = (unsigned char *)malloc(63);
    if (scb->response != NULL) {
        res = sign_dl_bmap((unsigned char *)(scb->response));
        if (res != 0) {
            free((char *)(scb->response));
            scb->response = NULL;
        }
        return res;
    } else {
        return -1;
    }
}

static int sc_ul(scmdblk *scb) {
    if (scb->data &&
        ((chainhdr *)(scb->data))->next == NULL &&
        ((chainhdr *)(scb->data))->size == 63) {
        return sign_ul_bmap(scb->data + sizeof(chainhdr));
    } else {
        return -1;
    }
}

static int sc_gprint (scmdblk *scb) {
#ifdef WIN32
  int remain;
  chainhdr *bp;
  char *p;

  if (!(scb->flags & SFLAG_APPEND)) {
    sign_clear();
  }

  bp = (chainhdr *)scb->data;
  if (bp == NULL || bp->size == 0) {
    /* Nothing to print */
    return xsc_printnull(scb);
  } else {
    p = (char *)bp + sizeof(chainhdr);
    remain = bp->size;
    return !i_wf_text("Lesspixels", 7, p, remain, scb);
  }
#else
    scb->result = -1;
    return -1;
#endif
}

/*
 * Responder functions: take data from scmdblk and respond with result of execution
 */

static int pscmdr_dl(scmdblk *cmd, SOCKET sock) {
  if (cmd->result != 0) {
    return -1;
  } else if (cmd->response != NULL) {
    sock_sendraw(sock, (char *)(cmd->response), 63);
    return 0;
  } else {
    return -1;
  }
}

static int pscmdr_stat(scmdblk *cmd, SOCKET sock) {
  return -1;
}

static int pscmdr_view(scmdblk *cmd, SOCKET sock) {
  unsigned char *c;
  int i, j, m;

  if (cmd->result != 0) {
    return -1;
  } else if (cmd->response != NULL) {
    c = ((unsigned char  *)cmd->response);
    for (i = 0; i < 7; i++) {
      for (j = 0; j < 9; j++) {
        m = 0x80;
        do {
          if (*c & m) {
            send(sock, "*", 1, 0);
          } else {
            send(sock, ".", 1, 0);
          }
          m >>= 1;
        } while (m);
        c++;
      }
      sock_sendeol(sock);
    }
    return 0;
  } else {
    return -1;
  }
}

int cmd_init(const char *device) {
  const char *defdev =
#if defined(WIN32)
    "COM8";
#elif defined(__CYGWIN__)
    "/dev/ttyS5";
#elif defined(__linux)
    "/dev/ttyUSB1";
#else
    "/dev/cu.serial1";
#endif

  /* Prolific drivers won't work with cheap piece of shit */
  /* Initialize LED sign */
  if (sign_open((device != NULL) ? device : defdev) != 0) {
    return -1;
  }

  sign_setabortpoll(cmd_cb_pollquit);
  return 0;
}

void cmd_cleanup(void) {
    sign_close();
}

int cmd_need_keepalive(void) {
    return sign_need_keepalive();
}

int cmd_call_keepalive(void) {
    return sign_call_keepalive();
}

void signd_icondblclick(HWND hwnd) {
    (void)hwnd;
}

/*
 * Data about commands for use by generic daemon code.
 */

/* Letters for sign commands */
const char *cmd_commands = "HNCFPRGDUSV";

signcmd cmd_cdata[] = {
    { NULL, NULL, sc_r_help,
      0,
      "Help" },
    { NULL, NULL, NULL,
      SFLAG_QUEUE,
      "(N)OP - No operation" },
    { NULL, sc_clear, NULL,
      SFLAG_QUEUE | SFLAG_TRANS | SFLAG_GNTEE,
      "(C)LEAR - Clear sign", },
    { NULL, sc_full, NULL,
      SFLAG_QUEUE | SFLAG_TRANS | SFLAG_GNTEE,
      "(F)ULL - Light all LEDs", },
    { sc_d_msg, sc_hprint, NULL,
      SFLAG_ASCIIZ | SFLAG_APPEND | SFLAG_LOOP | SFLAG_QUEUE |
      SFLAG_TRANS | SFLAG_GNTEE | SFLAG_ONCE,
      "H(P)RINT - Print using sign firmware" },
    { sc_d_msg, sc_sprint, NULL,
      SFLAG_ASCIIZ | SFLAG_APPEND | SFLAG_LOOP | SFLAG_QUEUE |
      SFLAG_TRANS | SFLAG_GNTEE | SFLAG_ONCE,
      "SP(R)INT - Print using software" },
    { sc_d_msg, sc_gprint, NULL,
      SFLAG_ASCIIZ | SFLAG_APPEND | SFLAG_LOOP | SFLAG_QUEUE |
      SFLAG_TRANS | SFLAG_GNTEE | SFLAG_ONCE,
      "(G)PRINT - Print using GDI" },
    { NULL, sc_dl, pscmdr_dl,
      0,
      "(D)L - Download image from sign", },
    { pscmd_ul, sc_ul, NULL,
      SFLAG_QUEUE | SFLAG_TRANS | SFLAG_GNTEE,
      "(U)L - Upload image to sign", },
    { NULL, NULL, pscmdr_stat,
      0,
      "(S)TATUS - Software status", },
    { NULL, sc_dl, pscmdr_view,
      0,
      "(V)IEW - Human-visible view",
    }
};
