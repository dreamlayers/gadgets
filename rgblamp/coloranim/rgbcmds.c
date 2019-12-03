/* Glue between sign daemon code and RGB lamps. */
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

#include "coloranim.h"
#include "signd.h"

/* size of allocation block for scmd->data */
#define CHAINBLK 10000

/*
 * Constant data
 */

/* Protocol version */
const char cmd_protocolid[] = "RGBlamp_1.0\r\n";

/*
 * Executor functions: perform the operation in scmdblk and put result in scmdblk
 */

/* Passing data from scmdblk to coloranim parser */

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

static int xsc_bfrtoken(char *b, int l, chainhdr **bp, char **p, int *remain) {
  int i;

  for (i = 0; i < l; i++) {
    if (*p == NULL) {
        return i;
    } else if (isspace(**p)) {
        do {
            xsc_nextchar(bp, p, remain);
        } while (*p != NULL && isspace(**p));
        return i;
    }
    b[i] = **p;
    xsc_nextchar(bp, p, remain);
  }

  return l;
}

static chainhdr *curanim_start, *curanim_bp;
static char *curanim_p;
static int curanim_remain, curanim_peeklen;
static char curanim_peekbuf[100], curanim_buf[100];
static int (*parse_fetch)(char *) = NULL;

static int parse_signd_fetch(char *buf)
{
    int l;

    if (curanim_p == NULL) return -1; /* parse past end of string */
    l = xsc_bfrtoken(buf, sizeof(curanim_buf) - 1,
                     &curanim_bp, &curanim_p, &curanim_remain);
    buf[l] = 0;
    return l;
}

static const char *parse_signd_peeknext(void)
{
    if (curanim_peeklen < 0) {
        curanim_peeklen = parse_fetch(curanim_peekbuf);
    }
    /* printf("PEEK: <%s>\n", curanim_peekbuf); */
    return (curanim_peeklen >= 0) ? curanim_peekbuf : NULL;
}

static const char *parse_signd_getnext(void)
{
    if (curanim_peeklen >= 0) {
        memcpy(curanim_buf, curanim_peekbuf, curanim_peeklen + 1);
        curanim_peeklen = -1;
        return curanim_buf;
    } else {
        int l = parse_fetch(curanim_buf);
        return (l >= 0) ? curanim_buf : NULL;
    }
}

static int parse_signd_eof(void)
{
    return curanim_p == NULL && curanim_peeklen < 0;
}

static void parse_signd_rewind(void)
{
    curanim_bp = curanim_start;
    curanim_p = (char *)curanim_bp + sizeof(chainhdr);
    curanim_remain = curanim_bp->size;
    curanim_peeklen = -1;
}

/*
 * Executor functions: perform the operation in scmdblk and put result in scmdblk
 */

static int sc_coloranim(scmdblk *scb)
{
    curanim_start = (chainhdr *)scb->data;
    parse_signd_rewind();

    parse_fetch = parse_signd_fetch;
    parse_getnext = parse_signd_getnext;
    parse_peeknext = parse_signd_peeknext;
    parse_eof = parse_signd_eof;
    parse_rewind = parse_signd_rewind;

    /* FIXME notify after one loop */
    return parse_main() ? -1 : 0;
}

static const char *parse_str_start, *parse_str_p;

static int parse_str_fetch(char *buf)
{
    int l;
    const char *start;
    if (*parse_str_p == 0) return -1;
    start = parse_str_p;
    while (*parse_str_p != 0 && !isspace(*parse_str_p)) {
        parse_str_p++;
    }
    l = parse_str_p - start;
    memcpy(buf, start, l);
    buf[l] = 0;
    while (isspace(*parse_str_p)) {
        parse_str_p++;
    }
    return l;
}

static int parse_str_eof(void)
{
    return *parse_str_p == 0;
}

static void parse_str_rewind(void)
{
    parse_str_p = parse_str_start;
    while (isspace(*parse_str_p)) {
        parse_str_p++;
    }
}

static int sc_preset(scmdblk *scb)
{
    int l;

    curanim_start = (chainhdr *)scb->data;
    parse_signd_rewind();
    l = parse_signd_fetch(curanim_buf);
    if (l <= 0) return -1;

    parse_str_start = preset_get(curanim_buf);
    if (parse_str_start == NULL) return -1;

    parse_fetch = parse_str_fetch;
    parse_getnext = parse_signd_getnext;
    parse_peeknext = parse_signd_peeknext;
    parse_eof = parse_str_eof;
    parse_rewind = parse_str_rewind;

    parse_str_rewind();
    return parse_main() ? -1 : 0;
}

int cmd_init(const char *device) {
    render_open();
    parse_init();
    mqtt_init();
    /* FIXME coloranim_setabortpoll(cmd_cb_pollquit); */
    return 0;
}

int cmd_clear(void) {
    /* FIXME do something */
    return 0;
}

void cmd_cleanup(void) {
    mqtt_quit();
    parse_quit();
    render_close();
}

int cmd_need_keepalive(void) {
    return 0;
}

int cmd_call_keepalive(void) {
    return 0;
}

/*
 * Data about commands for use by generic daemon code.
 */

/* Letters for sign commands */
const char *cmd_commands = "HNCP";

signcmd cmd_cdata[] = {
    { NULL, NULL, sc_r_help,
      0,
      "Help" },
    { NULL, NULL, NULL,
      SFLAG_QUEUE,
      "(N)OP - No operation" },
    { sc_d_msg, sc_coloranim, NULL,
      SFLAG_QUEUE | SFLAG_TRANS | SFLAG_GNTEE,
      "(C)oloranim - Send coloranim effect string to lamp" },
    { sc_d_msg, sc_preset, NULL,
      SFLAG_QUEUE | SFLAG_TRANS | SFLAG_GNTEE,
      "(P)reset - Send named coloranim preset to lamp" }
};
