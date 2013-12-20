/* Header file defining shared aspects of sign daemon protocol. */
/* Copyright 2013 Boris Gjenero. Released under the MIT license. */

#ifndef _SDPROTOCOL_H_

#define SPROTO_OK "OK\r\n"
#define SPROTO_QUEUED "QUEUED\r\n"
#define SPROTO_FAILED "FAILED\r\n"
#define SPROTO_DATA ":"
#define SPROTO_EOL "\r\n"
#define SPROTO_TIMEOUT "TIMEOUT\r\n"

#define SFLAG_CHARACTERS "ALZQTGO"

#define SFLAG_APPEND (1 << 0) /* Append rather than scrolling */
#define SFLAG_LOOP   (1 << 1) /* Keep looping message */
#define SFLAG_ASCIIZ (1 << 2) /* Null-terminated string */
#define SFLAG_QUEUE  (1 << 3) /* Queue only, don't wait for completion */
#define SFLAG_TRANS  (1 << 4) /* Message has transient time */
#define SFLAG_GNTEE  (1 << 5) /* Message has guaranteed time */
#define SFLAG_ONCE   (1 << 6) /* Message is guaranteed to be shown once */

#endif /* !_SDPROTOCOL_H_ */
