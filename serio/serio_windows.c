/* Windows code for the serio serial port library. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef _MSC_VER
#include <strsafe.h>
#else
#include <stdio.h>
#define StringCchPrintf snprintf
#endif

#define IN_SERIO
#include "serio.h"

/*** GLOBAL VARIABLES ***/

static HANDLE hComm;

/*** ERROR MANAGEMENT ***/

static void ErrorExit(LPTSTR lpszFunction)__attribute__ ((noreturn));
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
        (char *)lpszFunction, (int)dw, (char *)lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}

#if defined(__GNUC__)
void fatalerr(char *s) __attribute__ ((noreturn));
#elif defined(_MSC_VER)
__declspec(noreturn)
#endif
void fatalerr(char *s) {
  //MessageBox(NULL, s, "VFD client", MB_OK | MB_ICONERROR);
  ErrorExit(s);
}

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
}

/*** READING ***/

SERIO_LENGTH_RETURN serio_read(unsigned char *s, size_t l) {
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
}

/*** PORT CONTROL ***/

SERIO_RETURN serio_purge(unsigned int what) {
    DWORD flags = 0;

    if (what & SERIO_PURGEIN) flags |= PURGE_RXCLEAR;
    if (what & SERIO_PURGEOUT) flags |= PURGE_TXCLEAR;
    if (PurgeComm(hComm, flags) != TRUE) {
        SERIO_ERROR("serio_purge: PurgeComm failed");
    }
    SERIO_SUCCESS();
}

SERIO_RETURN serio_flush(void) {
    if (FlushFileBuffers(hComm) != TRUE) {
        SERIO_ERROR("serio_purge: FlushFileBuffers failed");
    }
    SERIO_SUCCESS();
}

/*** SETUP AND INITIALIZATION ***/

SERIO_RETURN serio_setreadtmout(unsigned int tmout) {
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
}

SERIO_RETURN serio_connect(const char *fname, unsigned int baud) {
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
    StringCchPrintf(modestr, sizeof(modestr), "%u,n,8,1", baud);
    FillMemory(&dcb, sizeof(dcb), 0);
    dcb.DCBlength = sizeof(dcb);
    if (!BuildCommDCB(modestr, &dcb)) {
        CloseHandle(hComm);
        SERIO_ERROR("serio_connect: BuildCommDCB failed");
    }
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
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

    serio_tcp = 0;
#ifdef SERIO_OVERLAPPED
    return xserio_initovl();
#else
    SERIO_SUCCESS();
#endif
}

void serio_disconnect(void) {
    CloseHandle(hComm);
}
