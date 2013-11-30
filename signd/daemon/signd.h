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

/* Descriptions of commands  */
extern const char *cmd_helptext[];

/* Letters for commands */
extern const char *cmd_commands;

/* Letters for command modifier flags */
extern const char *cmd_flags;

/* Array for all commands showing which flags are ok for that command */
extern const unsigned int cmd_flagmatrix[];

/* Arrays of functions for parameters, input, execution and responses */
extern int (*cmd_paramf[])(scmdblk *, SOCKET);
extern int (*cmd_inputf[])(scmdblk *, SOCKET);
extern int (*cmd_execf[])(scmdblk *);
extern int (*cmd_respf[])(scmdblk *, SOCKET);

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
