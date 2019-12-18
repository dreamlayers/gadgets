/* Header file for serio serial port library. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#ifndef _SERIO_H_
#define _SERIO_H_

#include <stddef.h>

/* User overlapped I/O so read and write can happen at the same time */
/* #define SERIO_OVERLAPPED */

/* All errors cause message output and program termination */
/* #define SERIO_ERRORS_FATAL */

/* Reads which fail to get the specified number of characters,
 * (typically due to timeout) will cause message output and
 * program termination. Writes are unaffected.
 */
/* #define SERIO_STRICT_READS */

/* Regular abort polling during reads and writes. */
#define SERIO_ABORT_POLL


#define SERIO_OK 0
#define SERIO_FAIL -1
#define SERIO_TIMEOUT -2
#define SERIO_ABORT -3

#ifdef SERIO_ERRORS_FATAL
/* No error returns. Errors cause message output and program termination. */
#define SERIO_RETURN void
#else
/* Non-negative means success or returns command-specific data */
#define SERIO_RETURN int
#endif

/* Abort polling interval */
#if defined(SERIO_ABORT_POLL) && !defined(SERIO_ABORT_POLL_MS)
#define SERIO_ABORT_POLL_MS 200
#endif

#if defined(SERIO_ERRORS_FATAL) && defined(SERIO_STRICT_READS)
/* There is never any need to return a value.
 * Errors cause message output and program termination.
 */
#define SERIO_LENGTH_RETURN void
#else
/* Returns success/failure or number of bytes transferred */
#define SERIO_LENGTH_RETURN int
#endif

/* Writes return:
 * 0 if all characters were written successfully
 * -1 otherwise
 */
SERIO_RETURN serio_putc(unsigned char c);
SERIO_RETURN serio_write(const unsigned char *s, size_t l);

/* Returns (non-negative) character or serio error code on failure */
int serio_getc(void);
/* If successful or timed out, returns number of characters read.
 * Otherwise, returns serio error code on failure.
 */
SERIO_LENGTH_RETURN serio_read(unsigned char *s, size_t l);

#define SERIO_PURGEIN 1
#define SERIO_PURGEOUT 2
SERIO_RETURN serio_purge(unsigned int what);

SERIO_RETURN serio_flush(void);

SERIO_RETURN serio_setreadtmout(unsigned int tmout);

SERIO_RETURN serio_connect(const char *fname, unsigned int baud);
SERIO_RETURN serio_connect_tcp(const char *address, unsigned int port);
SERIO_RETURN serio_connect_com(const char *fname, unsigned int baud);
void serio_disconnect(void);

void serio_setabortpoll(int (*func)(void));

int serio_secskalive(void);

#ifdef IN_SERIO
/* Definitions only visble to other parts of serio. */

#ifdef SERIO_ERRORS_FATAL
#define SERIO_ERROR(s) fatalerr(s)
#define SERIO_SUCCESS()
#define SERIO_PROPAGATE_ERROR(x) x
#else /* !SERIO_ERRORS_FATAL */
#define SERIO_ERROR(s) return SERIO_FAIL
#define SERIO_SUCCESS() return SERIO_OK
#define SERIO_PROPAGATE_ERROR(x) return x
#endif  /* !SERIO_ERRORS_FATAL */

#ifdef SERIO_ABORT_POLL
extern int (*abortpollf)(void);
#endif

extern int fd;
extern int serio_tcp;

#if defined(__GNUC__)
void fatalerr(char *s) __attribute__ ((noreturn));
#elif defined(_MSC_VER)
__declspec(noreturn) void fatalerr(char *s);
#else
void fatalerr(char *s);
#endif

void serio_disconnect_tcp(void);
void serio_disconnect_com(void);

#endif /* IN_SERIO */

#endif /* ifndef _SERIO_H_ */
