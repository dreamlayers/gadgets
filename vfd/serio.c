//#define SERIO_ERRORS_FATAL
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <time.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
//#include <pthread.h>
#include <errno.h>
#include <ctype.h>
#endif

#include "serio.h"

#ifdef WIN32
static HANDLE hComm;
#else
static int fd;
static int readtmout = 0;
#endif

static int (*abortpollf)(void);

/*** ERROR MANAGEMENT ***/

#ifndef SERIO_ERRORS_FATAL
#define SERIO_ERROR(s) return SERIO_FAIL
#define SERIO_SUCCESS() return SERIO_OK
#else /* SERIO_ERRORS_FATAL */
#define SERIO_ERROR(s) fatalerr(s)
#define SERIO_SUCCESS()

#ifdef WIN32
#include <strsafe.h>
static void ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}
#endif // WIN32

#if defined(__GNUC__)
static void fatalerr(char *s) __attribute__ ((noreturn));
#elif defined(_MSC_VER)
__declspec(noreturn)
#endif
static void fatalerr(char *s) {
#ifdef WIN32
  //MessageBox(NULL, s, "VFD client", MB_OK | MB_ICONERROR);
  ErrorExit(s);
#else
  fputs(s, stderr);
#endif
  exit(-1);
}

#endif // SERIO_ERRORS_FATAL

/*** OVERLAPPED (for simultaneous input and output) ***/

#ifdef SERIO_OVERLAPPED
static OVERLAPPED xserio_writeol;
static OVERLAPPED xserio_readol;

static int xserio_initovl(void) {
    // FIXME error reporting
    xserio_writeol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (xserio_writeol.hEvent == NULL) return FALSE;
    xserio_readol.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    return xserio_writeol.hEvent != NULL;
}

static void xserio_cleanupovl(void) {
    // FIXME
}

static BOOL xserio_write(HANDLE hFile,
                      LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                         LPDWORD lpNumberOfBytesWritten,
                         LPOVERLAPPED lpOverlapped) {
    if (!WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, &xserio_writeol)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            return GetOverlappedResult(hFile, &xserio_writeol, lpNumberOfBytesWritten, TRUE);
        } else {
            return FALSE;
        }
    } else {
        return TRUE;
    }
}

static BOOL xserio_read(HANDLE hFile,
                     LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                        LPDWORD lpNumberOfBytesRead,
                        LPOVERLAPPED lpOverlapped) {
    if (!ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, &xserio_readol)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            return GetOverlappedResult(hFile, &xserio_readol, lpNumberOfBytesRead, TRUE);
        } else {
            return FALSE;
        }
    } else {
        return TRUE;
    }
}
#else
#define xserio_write WriteFile
#define xserio_read ReadFile
#endif

/*** WRITING ***/

SERIO_RETURN serio_write(const unsigned char *s, size_t l) {
#ifdef WIN32
    DWORD written;
    if (!xserio_write(hComm, s, l, &written, NULL) ||
        written != l) {
        ClearCommError(hComm, NULL, NULL);
        SERIO_ERROR("serio_write: WriteFile failed");
    }
    if (written != l) {
        SERIO_ERROR("serio_write: short write");
    }
    SERIO_SUCCESS();
#else
    size_t remain = l;
    const unsigned char *p = s;

    while (remain > 0) {
        /* FIXME? Too much polling? */
        ssize_t res;

        struct timeval tv;
        fd_set fds;

        if (abortpollf != NULL && abortpollf()) {
            return SERIO_ABORT;
        }

        tv.tv_sec = 0;
        tv.tv_usec = SERIO_ABORT_POLL_US;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
     res = select(fd+1, NULL, &fds, NULL, &tv);
        if (res < 0) {
         SERIO_ERROR("serio_write: select failed");
        } else if (res == 0) {
            continue;
        }

     res = write(fd, p, remain);
        if (res <= 0 && errno != EAGAIN
#if EAGAIN != EWOULDBLOCK
            && errno != EWOULDBLOCK
#endif
            ){
            SERIO_ERROR("serio_write: write failed");
        }

        remain -= res;
        p += res;
    }

    SERIO_SUCCESS();
#if 0
    ssize_t written;

    written = write(fd, s, l);
    if (written != l) {
        SERIO_ERROR("serio_write: write failed\n");
    }
#endif // 0
#endif
}

SERIO_RETURN serio_putc(unsigned char c) {
#ifndef SERIO_ERRORS_FATAL
    return serio_write(&c, 1);
#else
    serio_write(&c, 1);
#endif
}

/*** READING ***/

SERIO_LENGTH_RETURN serio_read(unsigned char *s, size_t l) {
#ifdef WIN32
    DWORD dwRead;

    if (!xserio_read(hComm, s, l, &dwRead, NULL)) {
#ifdef SERIO_ERRORS_FATAL
        fatalerr("serio_read: ReadFile failed");
#else
        ClearCommError(hComm, NULL, NULL);
        return SERIO_FAIL;
#endif
    }

#ifdef SERIO_STRICT_READS
    if (dwRead != l) {
        fatalerr("serio_read: short read\n");
    }
#else
    return dwRead;
#endif
#else /* End of Windows code; Unix code follows */
    ssize_t res;
    struct timeval tv, totalwait;
    fd_set rfds;
    unsigned char *p = s;
    size_t remains = l;

    totalwait.tv_sec = readtmout / 1000;
    totalwait.tv_usec = (readtmout % 1000) * 1000;

    while (1) {
        if (abortpollf != NULL && abortpollf()) {
            return SERIO_ABORT;
        }

        tv.tv_sec = 0;
        if (totalwait.tv_sec == 0 && totalwait.tv_usec <= SERIO_ABORT_POLL_US) {
            tv.tv_usec = totalwait.tv_usec;
            totalwait.tv_usec = 0;
        } else {
            if (totalwait.tv_usec < SERIO_ABORT_POLL_US) {
                totalwait.tv_sec--;
                totalwait.tv_usec = 1000000 - SERIO_ABORT_POLL_US;
            } else {
                totalwait.tv_usec -= SERIO_ABORT_POLL_US;
            }
            tv.tv_usec = SERIO_ABORT_POLL_US;
        }

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        res = select(fd+1, &rfds, NULL, NULL, &tv);
        if (res == 1) {
            res = read(fd, p, remains);
            remains -= res;
            if (remains == 0) {
                /* Read requested amount successfully */
                return l;
            }
            p += res;
        } else if (res == 0) {
            if (totalwait.tv_usec == 0 && totalwait.tv_sec == 0) {
                /* Timeout */
                return l - remains;
            }
        } else {
            SERIO_ERROR("serio_read: select failed");
        }
    } /* while */
#endif /* End of serio_read Unix code */
}

int serio_getc(void) {
    unsigned char c;
#if defined(SERIO_STRICT_READS) && defined(SERIO_ERRORS_FATAL)
    serio_read(&c,1);
    return c;
#else
    int res;
    res = (int)serio_read(&c, 1);
    if (res == 1) {
        return c;
    } else if (res == 0) {
        return SERIO_TIMEOUT;
    } else {
        return (int)res;
    }
#endif
}

SERIO_RETURN serio_purge(unsigned int what) {
#ifdef WIN32
    DWORD flags = 0;

    if (what & SERIO_PURGEIN) flags |= PURGE_RXCLEAR;
    if (what & SERIO_PURGEOUT) flags |= PURGE_TXCLEAR;
    if (PurgeComm(hComm, flags) != TRUE) {
        SERIO_ERROR("serio_purge: PurgeComm failed");
    }
    SERIO_SUCCESS();
#else
    int flags = 0;

    if (what & SERIO_PURGEIN) {
        if (what & SERIO_PURGEOUT) {
        flags = TCIOFLUSH;
        } else {
            flags = TCIFLUSH;
        }
    } else if (what & SERIO_PURGEOUT) {
        flags = TCOFLUSH;
    }

    if (tcflush(fd, flags) != 0) {
        SERIO_ERROR("serio_purge: tcflush failed");
    } else {
        SERIO_SUCCESS();
    }
#endif
}

SERIO_RETURN serio_flush(void) {
#ifdef WIN32
    if (FlushFileBuffers(hComm) != TRUE) {
        SERIO_ERROR("serio_purge: FlushFileBuffers failed");
    }
    SERIO_SUCCESS();
#else
    if (tcdrain(fd) != 0) {
        SERIO_ERROR("serio_flush:tcdrain failed");
    }
    SERIO_SUCCESS();
#endif
}

/*** SETUP AND INITIALIZATION ***/

SERIO_RETURN serio_setreadtmout(unsigned int tmout) {
#ifdef WIN32
    COMMTIMEOUTS commtimeouts;

    /* Setup comm port timeouts (milliseconds) */
    commtimeouts.ReadIntervalTimeout = 0;
    commtimeouts.ReadTotalTimeoutMultiplier = 0;
    commtimeouts.ReadTotalTimeoutConstant = tmout;
    commtimeouts.WriteTotalTimeoutMultiplier = 0;
    commtimeouts.WriteTotalTimeoutConstant = 0;

    if (SetCommTimeouts(hComm, &commtimeouts) != TRUE) {
        SERIO_ERROR("serio_setreadtmout: SetCommTimeouts failed");
    }
    SERIO_SUCCESS();
#else
    readtmout = tmout;
    SERIO_SUCCESS();
#endif
}

SERIO_RETURN serio_connect(const char *fname, unsigned int baud) {
#ifdef WIN32
    char modestr[20];
    DCB dcb;

    /* Open comm port */
    hComm = CreateFile( fname,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        0,
                        OPEN_EXISTING,
#ifdef SERIO_OVERLAPPED
                        FILE_FLAG_OVERLAPPED,
#else
                        0 /* Not FILE_FLAG_OVERLAPPED */,
#endif
                        0);

    if (hComm == INVALID_HANDLE_VALUE) {
#ifdef SERIO_ERRORS_FATAL
        if (GetLastError() == ERROR_ACCESS_DENIED) {
            fatalerr("serio_connect: access denied");
        } else {
            fatalerr("serio_connect: CreatFile failed");
        }
#else
        return SERIO_FAIL;
#endif
    }

    /* Setup comm DCB */
    sprintf_s(modestr, sizeof(modestr), "%u,n,8,1", baud);
    FillMemory(&dcb, sizeof(dcb), 0);
    dcb.DCBlength = sizeof(dcb);
    if (!BuildCommDCB(modestr, &dcb)) {
        CloseHandle(hComm);
        SERIO_ERROR("serio_connect: BuildCommDCB failed");
    }
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = TRUE;

    /* Setup comm port using DCB */
    if (!SetCommState(hComm, &dcb)) {
        CloseHandle(hComm);
        SERIO_ERROR("serio_connect: SetCommState failed");
    }

/* In Windows 7, SetupComm can only increase the size of the input buffer. The output buffer is unaffected.
BOOL WINAPI SetupComm(
  __in  HANDLE hFile,
  __in  DWORD dwInQueue,
  __in  DWORD dwOutQueue
);
{ COMMPROP cp;
GetCommProperties(hComm, &cp);
fprintf(stderr, "Buf: i = %i (max = %i), o = %i (max = %i)\n", cp.dwCurrentRxQueue, cp.dwMaxRxQueue, cp.dwCurrentTxQueue, cp.dwMaxTxQueue);
}
*/

#ifdef SERIO_OVERLAPPED
    return xserio_initovl();
#else
    SERIO_SUCCESS();
#endif
#else // End of Windows code. Unix code follows
    struct termios temptermios;
    speed_t speed;

    /* OS X man page says speed corresponds to number (eg. B600 for 600 baud)
     Linux man page doesn't say that, so maybe it doesn't, and this is
     needed.
     */
    static const struct speedmap_s {
        unsigned int n;
        speed_t s;
    } speedmap[] = {
        { 600, B600 },
        { 1200, B1200 },
        { 9600, B9600 },
        { 0, 0 }
    };
    const struct speedmap_s *speedmap_p = speedmap;

    /* Find speed */
    while(1) {
        if (speedmap_p->n == 0) {
            SERIO_ERROR("serio_connect: unknown baud rate");
        } else if (speedmap_p->n == baud) {
            speed = speedmap_p->s;
            break;
        }
        speedmap_p++;
    }
    /* Open file */
    fd = open(fname, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        SERIO_ERROR("serio_open: open failed");
    }

  /* Get current tty settings and save them */
    if (tcgetattr(fd, &temptermios) != 0) {
        close(fd);
        SERIO_ERROR("serio_open: tcgetattr failed");
    }

    /* Set parameters */
    temptermios.c_iflag = IGNBRK;
    temptermios.c_oflag = 0;
    temptermios.c_cflag = CREAD | CLOCAL | CS8 | HUPCL;
    temptermios.c_lflag = 0;
    cfsetispeed(&temptermios, speed);
    cfsetospeed(&temptermios, speed);
    if (tcsetattr(fd, TCSAFLUSH, &temptermios) != 0) {
        close(fd);
        SERIO_ERROR("serio_open: tcsetattr failed");
    }

    SERIO_SUCCESS();
#endif
}

void serio_disconnect(void) {
#ifdef WIN32
    CloseHandle(hComm);
#else
    close(fd);
#endif
}

void serio_setabortpoll(int (*func)(void)) {
    abortpollf = func;
}
