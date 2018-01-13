#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "ws281xproxy.h"
#include "ws281xproto.h"

const ssize_t msglen[] = {
    0,
    sizeof(struct rgbmsg),
    sizeof(struct rgbmsg)
};

void fatal(const char *s) __attribute__ ((noreturn));
void fatal(const char *s)
{
    fprintf(stderr, "Fatal error: %s\n", s);
    exit(-1);
}

static int quitflag = 0;

static void termination_handler(int signo)
{
    switch (signo) {
    case SIGINT:
    case SIGHUP:
    case SIGQUIT:
    case SIGTERM:
        quitflag = 1;
    }
}

int main(void)
{
    struct sigaction new_action, old_action;

    mqd_t mq;
    struct rgbmsg msg;

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(msg);
    attr.mq_curmsgs = 0;


    /* Set up the structure to specify the new action. */
    new_action.sa_handler = termination_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction (SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGINT, &new_action, NULL);
    sigaction (SIGHUP, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &new_action, NULL);
    sigaction (SIGTERM, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGTERM, &new_action, NULL);

    umask(~(S_IRUSR | S_IWOTH));
    mq = mq_open("/ws281xproxy", O_RDONLY | O_CREAT, S_IRUSR | S_IWOTH, &attr);
    if (mq == (mqd_t)-1) {
        perror("Failed to create message queue");
        return -1;
    }
    render_open();

    while (!quitflag) {
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
            fprintf(stderr, "Got %i bytes but should have %i for message %i\n",
                    len, msglen[msg.type], msg.type);
            continue;
        }

        switch (msg.type) {
        case WSCMD_SRGB:
            render_1srgb(msg.rgb);
            break;
        case WSCMD_PWM:
            render_1pwm(msg.pwm);
            break;
        default:
            fprintf(stderr, "Message not handled\n");
            break;
        }

    }

    render_close();
}
