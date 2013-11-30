/* signcmds.h */

/* Sign commands */
typedef enum { SCMD_NOP,
               SCMD_CLEAR,
               SCMD_PRINT,
             } signcmd;

#define SCMD_MAX SCMD_PRINT

/* Command modifier flags */
#define SFLAG_APPEND   1               /* Append rather than scrolling */
#define SFLAG_LOOP     2               /* Keep looping message */
#define SFLAG_ASCIIZ   4               /* Null-terminated string */
#define SFLAG_QUEUE    8               /* Queue only, don't wait for completion */
#define SFLAG_TRANS    16              /* Message has transient time */
#define SFLAG_GNTEE    32              /* Message has guaranteed time */
#define SFLAG_ONCE     64              /* Message is guaranteed to be shown once */
