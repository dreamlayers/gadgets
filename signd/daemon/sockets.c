/* TCP server for sign gadgets. */
/* Copyright 2019 Boris Gjenero. Released under the MIT license. */

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
#include <signal.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define LPSOCKADDR (struct sockaddr *)
#define SOCKET int
#endif /* !WIN32 */

#include "signd.h"

/* Port to listen on */
#define PORT 9876
/* Number of connections to queue */
#define QUEUESIZE 5
/* Length of time a connection can wait for a result */
#define TMOUT_EXWAIT 100000

#ifdef WIN32
static HANDLE gnteetmr = NULL, transtmr[2];
#else
static struct timespec transtmr;
static struct timeval gnteetmr;
static int quit = 0; /* For quitting via signals */
#endif
static int wastrans = 0;

/* This permits abort polling from deep within the code driving the display,
 * without any need to know about and pass the scmdblk. It works because the
 * display can only work with one scmdblk at a time, but is a bit messy
 */
static scmdblk *curscb = NULL;

/* Protocol responses */
static const char mOk[] = SPROTO_OK;
static const char mQueued[] = SPROTO_QUEUED;
static const char mFailed[] = SPROTO_FAILED;
const char mData[] = SPROTO_DATA;
const char mEol[] = SPROTO_EOL;
static const char mTIMEOUT[] = SPROTO_TIMEOUT;

/* Command queue */
scmdblk *qhead = NULL, *qtail = NULL;
#ifdef WIN32
CRITICAL_SECTION qcs;
HANDLE qevent;
#else
pthread_mutex_t qevent_mtx;
pthread_cond_t qevent;
#endif

static void initq() {
#ifdef WIN32
  InitializeCriticalSection(&qcs);
  qevent = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
  pthread_mutex_init(&qevent_mtx, NULL);
  pthread_cond_init(&qevent, NULL);
#endif
  qhead = NULL;
  qtail = NULL;
}

static void enqcmd(scmdblk *cmd) {
#ifdef WIN32
  EnterCriticalSection(&qcs);
#else
  pthread_mutex_lock(&qevent_mtx);
#endif
  cmd->next = NULL;
  if (qhead == NULL) {
    cmd->prev = NULL;
    qhead = cmd;
    qtail = cmd;
  } else {
    qhead->next = cmd;
    cmd->prev = qhead;
    qhead = cmd;
  }
#ifdef WIN32
  LeaveCriticalSection(&qcs);
  SetEvent(qevent);
#else
  pthread_cond_signal(&qevent);
  pthread_mutex_unlock(&qevent_mtx);
#endif
}

static scmdblk *deqcmd() {
    scmdblk *r;

#ifdef WIN32
    EnterCriticalSection(&qcs);
#endif
    /* In Pthreads, condition waits unlock the mutex while waiting.
     * It is then more efficient to have higher level code handle the mutex.
     */

    r = qtail;
    if (qtail != NULL) {
        qtail = qtail->next;
        if (qtail != NULL) {
            qtail->prev = NULL;
        } else {
            qhead = NULL;
        }
    }

#ifdef WIN32
    LeaveCriticalSection(&qcs);
#endif

    return r;
}

static int qempty() {
    return qtail == NULL;
}

static void fatalerr(char *msg) {
    fprintf(stderr, "%s\n", msg);
#ifdef WIN32
    WSACleanup();
#endif
    exit(-1);
}

static void freescb(scmdblk *scb) {
#ifndef WIN32
  int val;
#endif
  if (scb == NULL) return;
#ifdef WIN32
  if (InterlockedDecrement((LPLONG)&(scb->refcnt)) == 0) {
    if (scb->event) CloseHandle(scb->event);
#else
  pthread_mutex_lock(&scb->refcnt_mtx);
  scb->refcnt--;
  val = scb->refcnt;
  pthread_mutex_unlock(&scb->refcnt_mtx);
  if (val == 0) {
    if (!(scb->flags & SFLAG_QUEUE)) {
      pthread_cond_destroy(&scb->event);
      pthread_mutex_destroy(&scb->event_mtx);
    }
    pthread_mutex_destroy(&scb->refcnt_mtx);

#endif
    if (scb->response) free((char *)(scb->response));  /* de-volatile */
    if (scb->data) cmd_freedata(scb->data);
    free(scb);
  }
}

static scmdblk *scb_alloc_init(void) {
    scmdblk *scb;
    
    scb = (scmdblk *)malloc(sizeof(scmdblk));
    scb->flags = 0;
#ifndef WIN32
    scb->done = false;
#endif
    scb->data = NULL;
    scb->response = NULL;
#ifdef WIN32
    scb->event = NULL;
#endif
    scb->refcnt = 1;
#ifndef WIN32
    pthread_mutex_init(&scb->refcnt_mtx, NULL);
#endif
    return scb;
}

#ifdef WIN32
static DWORD WINAPI client(LPVOID lpParameter) {
  SOCKET sock;
#else
static void *client(void *lpParameter) {
  int sock;
#endif
  int nRet, t;
  char c;
  char *cp;
  scmdblk *scb = NULL;
  int res = -1;

#ifdef WIN32
  sock = (SOCKET)lpParameter;

  /* Make sure socket is blocking */
  WSAAsyncSelect(sock, hWnd, 0, 0);
#else
  sock = (intptr_t)lpParameter;
#endif

  t = 0;
#ifdef WIN32
  nRet = ioctlsocket(sock, FIONBIO, (u_long FAR*) &t);
#else
  nRet = ioctl(sock, FIONBIO, &t);
#endif

  if (nRet != 0) {
    /* printf("Error on ioctlsocket()"); */
    goto clifault2;
  }

  /* Send protocol version */
  nRet = (int)send(sock, cmd_protocolid, strlen(cmd_protocolid), 0);
  if (nRet == SOCKET_ERROR) {
    /* printf("Error on send()"); */
    goto clifault2;
  }

  while (1) {
    scb = scb_alloc_init();

    /* Skip to command */
    while (1) {
      if (recv(sock, &c, 1, 0) != 1) goto clifault;
      if (!isspace(c)) break;
    }

    /* Translate command */
    cp = strchr(cmd_commands, toupper(c));
    if (cp == NULL) goto clifault;
    scb->cmd = cp - cmd_commands;

    /* Skip space after command */
    while (1) {
      if (recv(sock, &c, 1, 0) != 1) goto clifault;
      if (c != ' ' && c != '\t') break;
    }

    if (c == '/') {
      while(1) {
        if (recv(sock, &c, 1, 0) != 1) goto clifault;

        if (c == ' ' || c == '\t') continue;    /* Skip whitespace */

        if (c == ':' || c == '\r' || c == '\n') break;

        /* Generate mask */
        cp = strchr(cmd_flags, toupper(c));
        if (cp == NULL) goto clifault;
        t = (int)(cp - cmd_flags);
        scb->flags |= 1 << t;
        if (cmd_fdata[t].paramf != NULL) {
          if (cmd_fdata[t].paramf(scb, sock) != 0) goto clifault;
        }

        /* Test mask */
        if (scb->flags & ~cmd_cdata[scb->cmd].flags) goto clifault;
      }
    }

    if (c == ':') {
      if (cmd_cdata[scb->cmd].inputf == NULL) goto clifault;
      /* Get data from client */
      if (cmd_cdata[scb->cmd].inputf(scb, sock) != 0) goto clifault;
    } else {
      if (c != '\r' && c != '\n') goto clifault;
    }

    /* If we have to wait then create the event we will wait on */
    if (!(scb->flags & SFLAG_QUEUE)) {
#ifdef WIN32
      scb->event = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (scb->event == NULL) goto clifault;
#else
      pthread_mutex_init(&scb->event_mtx, NULL);
      pthread_cond_init(&scb->event, NULL);
#endif
    }
    /* Enqueue command */
    scb->refcnt++;  /* Still only known from this end, so no need for mutex */
    enqcmd(scb);

    /* If we have to wait then wait */

    if (scb->flags & SFLAG_QUEUE) {
      res = 0;
      send(sock, mQueued, sizeof(mQueued), 0);
    } else {
#ifdef WIN32
      if (WaitForSingleObject(scb->event, TMOUT_EXWAIT) == WAIT_TIMEOUT) {
#else
      struct timeval tp;
      struct timespec tmo;

      gettimeofday(&tp, NULL);
      tmo.tv_sec = tp.tv_sec + TMOUT_EXWAIT/1000;
      tmo.tv_nsec = tp.tv_usec * 1000;

      pthread_mutex_lock(&scb->event_mtx);

      res = 0;
      while(1) {
        if (scb->done) break;
        res = pthread_cond_timedwait(&scb->event, &scb->event_mtx, &tmo);
        if (res == EBUSY) break;
      }

      if (res) {
#endif
        send(sock, mTIMEOUT, sizeof(mTIMEOUT), 0);
      } else {
        /* Respond to client */
        if (cmd_cdata[scb->cmd].respf != NULL) {
            res = cmd_cdata[scb->cmd].respf(scb, sock);
        } else {
            res = scb->result;
        }

        /* Final OK / FAILED response */
        if (res == 0) {
            send(sock, mOk, sizeof(mOk), 0);
        } else {
            send(sock, mFailed, sizeof(mFailed), 0);
        }
      }
#ifndef WIN32
      pthread_mutex_unlock(&scb->event_mtx);
#endif
    }

    /* free the scb from this end */
    freescb(scb);
    scb = NULL;
  }
clifault:
  scb->flags |= SFLAG_QUEUE; /* Where these errors occur, those aren't allocated */
  freescb(scb);
  /* Close socket to client */
clifault2:
#ifdef WIN32
  closesocket(sock);
#else
  close(sock);
#endif

  return 0;
}

#ifdef WIN32
WSADATA wsaData;
SOCKET lsock;
#else
int lsock;
#endif

#if 0
#include <io.h>
void getconsole() {
    int hCrt, i;
    FILE *hf;

    AllocConsole();
    hCrt = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), 0 /* _O_TEXT */);
    hf = _fdopen( hCrt, "w" );
    *stderr = *hf;
    i = setvbuf( stdout, NULL, _IONBF, 0 );

    fprintf(stderr, "Debug output:\n");
}
#else
#define getconsole()
#endif

int init_socket(const char *device) {
#ifdef WIN32
  WORD version = MAKEWORD(1,1);
  SOCKADDR_IN saServer;
  HANDLE tempH;
  DWORD ThreadId;
#else
  struct sockaddr_in saServer;
  pthread_t signthread;
#endif
  int nRet;

  getconsole();

  if (cmd_init(device) != 0) {
      fatalerr("Error initializing hardware");
  }

#ifdef WIN32
  /* The sign is initalized first because it may take some time to initialize */
  tempH = CreateThread(NULL, 0, &signproc, NULL, 0, &ThreadId);
  if (tempH == NULL) {
    fatalerr("Error at CreateThread() for sign");
  }
  /* Increase sign thread priority */
  SetThreadPriority(tempH, THREAD_PRIORITY_TIME_CRITICAL);
#else
  nRet = pthread_create(&signthread, NULL, &signproc, NULL);
  if (nRet) {
    fatalerr("Error at pthread_create() for sign");
  }
  // FIXME priority
#endif

  /* Initialize the queue */
  initq();

#ifdef WIN32
  /* Start Winsock */
  WSAStartup(version, &wsaData);
#endif

  /* Create listening socket */
  lsock = socket(AF_INET,               /* IP */
                 SOCK_STREAM,           /* Socket type */
                 IPPROTO_TCP);          /* Protocol */

  if (lsock == INVALID_SOCKET) {
    fatalerr("Error at socket()");
  }

  /* Fill out address */
  memset(&saServer, 0, sizeof(saServer));
  saServer.sin_family = AF_INET;
  /* Since this is a server, any address will do */
  saServer.sin_addr.s_addr = INADDR_ANY;
  saServer.sin_port = htons(PORT);

  /* Bind the socket to our local server address */
#ifdef WIN32
  nRet = bind(lsock, (LPSOCKADDR)&saServer, sizeof(struct sockaddr));
#else
  nRet = bind(lsock, (struct sockaddr *)&saServer, sizeof(struct sockaddr));

#endif
  if (nRet == SOCKET_ERROR) {
    fatalerr("Error at bind()");
  }

  /* Make the socket listen */
  nRet = listen(lsock, QUEUESIZE);
  if (nRet == SOCKET_ERROR) {
    fatalerr("Error at listen()");
  }

#ifdef WIN32
  /* Set up messages for accept */
  /* FIXME: is this needed? */
  if (WSAAsyncSelect(lsock, hWnd, ACCEPT_EVENT, FD_ACCEPT) != 0) {
    fatalerr("Error at WSAAsyncSelect()");
  }
#endif

  return 0;
}

int accept_client(void) {
#ifdef WIN32
  SOCKET csock;
  DWORD ThreadId;
  HANDLE TempH;
#else
  intptr_t csock;
  pthread_t client_thread;
#endif

  csock = accept(lsock,
                 NULL,    /* Address of a sockaddr structure (see below) */
                 NULL);    /* Address of a variable containing the size of sockaddr */
  if (csock == INVALID_SOCKET) {
#ifndef WIN32
    if (quit && errno == EINTR) return 0;
#endif
    fatalerr("Error at accept()");
  }

#ifdef WIN32
  TempH = CreateThread(NULL, 10000, &client, (LPVOID)csock, 0, &ThreadId);
  if (TempH == NULL) {
    fatalerr("Error at CreateThread() for client");
  }
  CloseHandle(TempH);
#else
  if (pthread_create(&client_thread, NULL, &client, (void *)csock)) {
    fatalerr("Error at pthread_create for client");
  }
#endif

  return 0;
}

void cleanup_socket(void) {
  /* Close listening socket */
#ifdef WIN32
  closesocket(lsock);
  /* Shutdown Winsock */
  WSACleanup();
#else
  close(lsock);
#endif

  cmd_cleanup();
}

int sock_sendraw(SOCKET sock, char *data, unsigned int l) {
    send(sock, mData, sizeof(mData), 0);
    send(sock, data, l, 0);
    return 0;
}

int sock_sendeol(SOCKET sock) {
    send(sock, mEol, sizeof(mEol), 0);
    return 0;
}

/*
 * Main thread
 */
int cmd_cb_pollquit(void) {
#ifndef WIN32
  if (quit) return 1;
#endif
  if (curscb == NULL) return 0;

  if ((curscb->flags & SFLAG_GNTEE) && (curscb->gnteetime > 0)) {
#ifdef WIN32
    /* Still maybe guaranteed */

    if (WaitForSingleObject(gnteetmr, 0) == WAIT_OBJECT_0) {
      /* Guarantee expired */
      curscb->gnteetime = 0;
    } else {
      /* Still guaranteed */
      return 0;
    }
#else
    struct timeval tp;

    gettimeofday(&tp, NULL);
    if (tp.tv_sec < gnteetmr.tv_sec ||
        (tp.tv_sec == gnteetmr.tv_sec &&
         tp.tv_usec <= gnteetmr.tv_usec))
      return 0;
#endif
  }

  if (!qempty()) {
    /* We have a new command */
    return 1;
  }

#ifdef WIN32
  if (wastrans && WaitForSingleObject(transtmr[0], 0) == WAIT_OBJECT_0)
    /* Transience expired */
    return 1;
#else
  if (wastrans) {
    struct timeval tp;

    gettimeofday(&tp, NULL);
    if (tp.tv_sec > transtmr.tv_sec ||
        (tp.tv_sec == transtmr.tv_sec &&
         tp.tv_usec * 1000 >= transtmr.tv_nsec))
      return 1;
  }
#endif

  /* No guarantee but no reason to end now either */
  return 0;
}

#ifdef WIN32
DWORD WINAPI signproc(LPVOID lpParameter) {
  LARGE_INTEGER liDueTime;
#else
void *signproc(void *lpParameter) {
#endif
  scmdblk *scb, *nontransscb = NULL, *tscb = NULL;

#ifdef WIN32
  /* Create timers we need for guarantee and transience */
  gnteetmr = CreateWaitableTimer(NULL, TRUE, NULL);
  if (!gnteetmr) {
    fatalerr("CreateWaitableTimer failed.\n");
  }
  transtmr[0] = CreateWaitableTimer(NULL, TRUE, NULL);
  if (!transtmr[0]) {
    fatalerr("CreateWaitableTimer failed.\n");
  }
  transtmr[1] = qevent;
#endif

  /* Begin accepting commands */
  while (1) {
    /* Get command off queue */
#ifndef WIN32
    pthread_mutex_lock(&qevent_mtx);
    /* In pthreads, cond wait functions unlock the mutex while waiting. */
#endif
    while (1) {
      scb = deqcmd();
      if (scb != NULL) break;

      if (wastrans) {
#ifdef WIN32
        if (WaitForMultipleObjects(2, transtmr, FALSE, INFINITE) == WAIT_OBJECT_0) {
#else
        if (pthread_cond_timedwait(&qevent, &qevent_mtx, &transtmr) == ETIMEDOUT) {
#endif
          /* Transience expired */
          wastrans = 0;
          if (nontransscb == NULL) {
            cmd_clear();
          } else {
            scb = nontransscb;
            break;
          }
        }
      } else {
        if (cmd_need_keepalive()) {
          /* Keep alive handling is not perfect. Transience and guarantee
             times could defeat it, and it is incompatible with situations
             where sending a character aborts sign operation. */
#ifdef WIN32
          if (WaitForSingleObject(qevent, 60 * 1000) == WAIT_TIMEOUT)
#else
          struct timeval tp;

          gettimeofday(&tp, NULL);
          transtmr.tv_sec = tp.tv_sec + 60;
          transtmr.tv_nsec = tp.tv_usec * 1000;
          if (transtmr.tv_nsec > 1000000000) {
            transtmr.tv_nsec -= 1000000000;
            transtmr.tv_sec++;
          }
          if (pthread_cond_timedwait(&qevent, &qevent_mtx, &transtmr) == ETIMEDOUT)
#endif
          {
            cmd_call_keepalive();
          }
        } else {
#ifdef WIN32
          WaitForSingleObject(qevent, INFINITE);
#else
          pthread_cond_wait(&qevent, &qevent_mtx);
#endif
        }
      }
    } /* while (1) */
#ifndef WIN32
    pthread_mutex_unlock(&qevent_mtx);
#endif

    curscb = scb; /* Remember for sc_abortpoll */

    /* Start guarantee or transience clock if applicable */
    if ((scb->flags & SFLAG_GNTEE) && (scb->gnteetime > 0)) {
#ifdef WIN32
      liDueTime.QuadPart = scb->gnteetime;
      liDueTime.QuadPart *= -100000;      /* Our unit was 1/100 s, Win32 is 100 ns */
      if (!SetWaitableTimer(gnteetmr, &liDueTime, 0, NULL, NULL, 0)) {
        fatalerr("SetWaitableTimer failed.\n");
      }
#else
      gettimeofday(&gnteetmr, NULL);
      gnteetmr.tv_sec += scb->gnteetime / 100;
      gnteetmr.tv_usec += (scb->gnteetime % 100) * 10000;
#endif
    }

    if (scb->flags & SFLAG_TRANS) {
#ifdef WIN32
      liDueTime.QuadPart = scb->transtime;
      liDueTime.QuadPart *= -100000;      /* Our unit was 1/100 s, Win32 is 100 ns */
      if (!SetWaitableTimer(transtmr[0], &liDueTime, 0, NULL, NULL, 0)) {
        fatalerr("SetWaitableTimer failed.\n");
      }
#else
      struct timeval tp;

      gettimeofday(&tp, NULL);
      transtmr.tv_sec = tp.tv_sec + scb->transtime / 100;
      /* Our unit is 1/100, pthread is 1/1000000000 */
      transtmr.tv_nsec = tp.tv_usec * 1000 + scb->transtime % 100 * 10000000;
      if (transtmr.tv_nsec > 1000000000) {
        transtmr.tv_nsec -= 1000000000;
        transtmr.tv_sec++;
      }
#endif
      wastrans = 1;
    } else {
      wastrans = 0;
      if (scb != nontransscb) {
        /* If scb is non-transient remember last non-transient */
        tscb = nontransscb;
        nontransscb = scb;
        if (tscb != NULL) {
          freescb(tscb);
        }
      }
    }

    /* Execute command */
    if (cmd_cdata[scb->cmd].execf != NULL) {
        scb->result = cmd_cdata[scb->cmd].execf(scb);
    } else {
        scb->result = 0;
    }

    /* Notify of completion (maybe already notified) */
#ifdef WIN32
    if (scb->event != NULL) {
#else
    if (!(scb->flags & SFLAG_QUEUE)) {
#endif
#ifdef WIN32
      SetEvent(scb->event);
#else
      pthread_mutex_lock(&scb->event_mtx);
      scb->done = true;
      pthread_cond_signal(&scb->event);
      pthread_mutex_unlock(&scb->event_mtx);
#endif
    }

    /* Await guarantee time */
    if ((scb->flags & SFLAG_GNTEE) && (scb->gnteetime > 0)) {
#ifdef WIN32
      WaitForSingleObject(gnteetmr, INFINITE);
#else
      struct timeval tp;
      gettimeofday(&tp, NULL);
      if (tp.tv_sec < gnteetmr.tv_sec ||
        (tp.tv_sec == gnteetmr.tv_sec &&
         tp.tv_usec <= gnteetmr.tv_usec))
        usleep((unsigned int)((gnteetmr.tv_sec - tp.tv_sec) * 1000000 +
               (gnteetmr.tv_usec - tp.tv_usec)));
#endif
    }

    /* Dereference and maybe free the SCB */
    if (scb != nontransscb) {
      freescb(scb);
    }
  }

  return 0;
}

void cmd_cb_notify(scmdblk *scb) {
      /* Notify of completion */
#ifdef WIN32
      if (scb->event != NULL) {
#else
      if (!(scb->flags & SFLAG_QUEUE)) {
#endif
        scb->result = 0;
#ifdef WIN32
        SetEvent(scb->event);
#else
        pthread_mutex_lock(&scb->event_mtx);
        scb->done = true;
        pthread_cond_signal(&scb->event);
        pthread_mutex_unlock(&scb->event_mtx);
#endif
      }
}

#ifndef WIN32
static void sig_quit(int sig) {
    quit = 1;
}

static void sig_setup(void) {
    struct sigaction sa;

    memset (&sa, 0, sizeof(sa));
    sa.sa_handler = sig_quit;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(int argc, char **argv) {

    if (argc != 1 && argc != 2) {
        fprintf(stderr, "USAGE: %s [DEVICE]\n", argv[0]);
    }
    sig_setup();
    /* FIXME Don't treat EINTR errors as fatal */
    init_socket((argc == 2) ? argv[1] : NULL);
    while (!quit) { accept_client(); }
    cleanup_socket();
}
#endif /* !WIN32 */

void cmd_enq_string(int cmd, char *data, unsigned int len) {
    scmdblk *scb;
    chainhdr *ch;

    scb = scb_alloc_init();
    scb->flags |= SFLAG_QUEUE;
    scb->cmd = cmd;

    /* TODO: Support flags */
    
    /* Copy data */
    if (len > 0) {
        ch = (chainhdr *)malloc(sizeof(chainhdr) + len);
        ch->next = NULL;
        ch->size = len;
        memcpy(((char *)ch) + sizeof(chainhdr), data, len);
        scb->data = (unsigned char *)ch;
    }

    /* Enqueue command */
    scb->refcnt++;  /* Still only known from this end, so no need for mutex */
    enqcmd(scb);

    /* free the scb from this end */
    freescb(scb);
}
