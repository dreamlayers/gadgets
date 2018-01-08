#include <stdlib.h>
#include <stdio.h>
#include <mqueue.h>
#include "ws281xproxy.h"
#include "ws281xproto.h"

const ssize_t msglen[] = { sizeof(struct rgbmsg) };

void fatal(const char *s) __attribute__ ((noreturn));
void fatal(const char *s)
{
    fprintf(stderr, "Fatal error: %s\n", s);
    exit(-1);
}

int main(void)
{
    mqd_t mq;
    struct rgbmsg msg;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(msg);
    attr.mq_curmsgs = 0;

    umask(~(S_IRUSR | S_IWOTH));
    mq = mq_open("/ws281xproxy", O_RDONLY | O_CREAT, S_IRUSR | S_IWOTH, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create message queue");
        return -1;
    }
    render_open();

    while (1) {
        ssize_t len = mq_receive(mq, (char *)&msg, sizeof(msg), NULL);
        if (len < 0) {
            perror("Failed to receive message");
            continue;
        }
        if (len < sizeof(unsigned int)) {
            fprintf(stderr, "Unidentifiable short message received\n");
            continue;
        }
        if (msg.type > (sizeof(msglen) / sizeof(msglen[0]))) {
            fprintf(stderr, "Unknown message received\n");
            continue;
        }
        if (len != msglen[msg.type]) {
            fprintf(stderr, "Invalid length message received\n");
            continue;
        }

        switch (msg.type) {
        case 0:
            render1(msg.rgb);
            break;
        default:
            fprintf(stderr, "Message not handled\n");
            break;
        }

    }
}
