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

/* Command modifier flags */
#define SFLAG_APPEND (1 << 0) /* Append rather than scrolling */
#define SFLAG_LOOP   (1 << 1) /* Keep looping message */
#define SFLAG_ASCIIZ (1 << 2) /* Null-terminated string */
#define SFLAG_QUEUE  (1 << 3) /* Queue only, don't wait for completion */
#define SFLAG_TRANS  (1 << 4) /* Message has transient time */
#define SFLAG_GNTEE  (1 << 5) /* Message has guaranteed time */
#define SFLAG_ONCE   (1 << 6) /* Message is guaranteed to be shown once */

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

#if 0
    FIXME delete
/* Queue functions */
void enqcmd(scmdblk *cmd);
scmdblk *deqcmd();
int qempty();
#ifdef WIN32
extern HANDLE qevent;
#else
extern pthread_mutex_t qevent_mtx;
extern pthread_cond_t qevent;
#endif
#endif

#ifdef WIN32
/* Message */
#define ACCEPT_EVENT (WM_USER+2)

extern HWND hWnd;

//#include "tray42.h"
#endif

int init_socket(void);
int accept_client(void);
void cleanup_socket(void);

/* Protocol version: sent to client to identify server protocol */
extern const char cmd_protocolid[];

/* Letters for commands */
extern const char *cmd_commands;

/* Data about commands */
extern signcmd cmd_cdata[];

/* Letters for command modifier flags */
extern const char *cmd_flags;

/* Arrays of functions for parameters */
extern int (*cmd_paramf[])(scmdblk *, SOCKET);

/* Called to free scmdblk->data */
void cmd_freedata(void *p);

int cmd_init(void);
void cmd_cleanup(void);

int cmd_pollquit(void);

void cmd_notify(scmdblk *scb);

int cmd_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* _SIGND_H_ */
