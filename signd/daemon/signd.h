/* Sign daemon */

#ifndef _SIGND_H_
#define _SIGND_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#ifdef _MSC_VER
typedef enum {
    false = 0,
    true = 1
} bool;
#else
#include <stdbool.h>
#endif
#endif

#include "../sdprotocol.h"

/* size of allocation block for scmd->data */
#define CHAINBLK 10000

/* Protocol responses */
int sock_sendraw(SOCKET sock, char *data, unsigned int l);
int sock_sendeol(SOCKET sock);

/* Chain of data areas */
struct chainhdr_s;

typedef struct chainhdr_s {
  struct chainhdr_s *next;
  unsigned int size;
} chainhdr;

struct scmdblk_s;

/* Command descriptor block */
typedef struct scmdblk_s {
  int cmd;
  unsigned int flags;
  struct scmdblk_s *prev, *next;
  unsigned char *data;
  volatile void (*freedata)(unsigned char *);
  volatile unsigned char *response;
  volatile int result;
#ifdef WIN32
  HANDLE event;
  LONG volatile refcnt;
#else
  pthread_mutex_t event_mtx;
  pthread_cond_t event;
  unsigned int refcnt;
  pthread_mutex_t refcnt_mtx;
  bool done;
#endif
  unsigned long transtime, gnteetime;
} scmdblk;

/* Data about flags, supplied by device specific code */
typedef struct signflag_s {
    /* Obtains parameter, or NULL if no parameter */
    int (*paramf)(scmdblk *, SOCKET);

    const char *helptext;
} signflag;

/* Data about commands, supplied by device specific code */
typedef struct signcmd_s {
    /* Reads data from socket and stores it in memory */
    int (*inputf)(scmdblk *, SOCKET);
    /* Reads command from memory and executes it on the sign */
    int (*execf)(scmdblk *);
    /* Reads response from memory and returns it to the client */
    int (*respf)(scmdblk *, SOCKET);

    /* Which flags are allowed for this command? */
    unsigned int flags;

    const char *helptext;
} signcmd;

/* Sign thread */
#ifdef WIN32
DWORD WINAPI signproc(LPVOID lpParameter);
#else
void *signproc(void *lpParameter);
#endif

#ifdef WIN32
/* Message */
#define ACCEPT_EVENT (WM_USER+2)

extern HWND hWnd;

//#include "tray42.h"
#endif

int init_socket(const char *device);
int accept_client(void);
void cleanup_socket(void);

/*
 * Data provided by application-specific code, for customizing the daemon.
 */

/* Protocol version: sent to client to identify server protocol */
extern const char cmd_protocolid[];

/* Letters for commands */
extern const char *cmd_commands;

/* Data about commands */
extern signcmd cmd_cdata[];

/* Letters for command modifier flags */
extern const char *cmd_flags;

/* Data about flags */
extern signflag cmd_fdata[];

/*
 * API between daemon core and application-specific code
 */

/* Called by daemon at startup to initialize application-specific things */
int cmd_init(const char *device);

/* Called by daemon at exit to clean up application-specific things */
void cmd_cleanup(void);

/* Called by daemon when a transient message expires
 * and there is nothing else to display.
 */
int cmd_clear(void);

/* Called by daemon to free scmdblk->data */
void cmd_freedata(void *p);

/* The following callbacks are optional. They are meant for
 * where execf() does not return promptly.
 */

/* Called by application-specific code to poll daemon
 * whether current command should end.
 */
int cmd_cb_pollquit(void);

/* Called by application-specific code to tell client that
 * command has finished and return result code.
 */
void cmd_cb_notify(scmdblk *scb);

/*
 * Utility functions available in gencmds.c
 */

/* Free chain such as scb->data */
void cmd_freedata(void *p);

/* Read unsigned long from socket, typically for parameters */
int sc_read_ulong(SOCKET sock, unsigned long *out);

/*
 * Generic commands available in gencmds.c
 */

/* Data of arbitrary length for a message command */
int sc_d_msg(scmdblk *cmd, SOCKET sock);

/* Respond with help information */
int sc_r_help(scmdblk *scb, SOCKET sock);

#ifdef __cplusplus
}
#endif

#endif /* _SIGND_H_ */
