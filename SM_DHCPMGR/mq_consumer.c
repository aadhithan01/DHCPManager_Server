#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include "sm_DhcpMgr.h"
#include "../DHCP_RBUS_COM/dhcpmgr_rbus_apis.h"

static volatile int keep_running = 1;

static void sigint_handler(int signum)
{
    (void)signum;
    keep_running = 0;
}

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    struct mq_attr attr;
    mqd_t mq;
    ssize_t n;
    DhcpManagerEvent evt;

    signal(SIGINT, sigint_handler);

    /* Ensure the queue exists. Create if missing with matching attributes. */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(DhcpManagerEvent);
    attr.mq_curmsgs = 0;

    mq = mq_open(MQ_NAME, O_RDONLY | O_CREAT, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open (consumer)");
        return EXIT_FAILURE;
    }

    printf("MQ consumer listening on %s (msgsize=%ld)\n", MQ_NAME, (long)attr.mq_msgsize);

    while (keep_running) {
        n = mq_receive(mq, (char*)&evt, sizeof(evt), NULL);
        if (n >= 0) {
            printf("Received event %d from MQ\n", evt);
            /* Dispatch to the DHCP state machine */
            DispatchDHCP_SM((DhcpManagerEvent)evt, NULL);
        } else {
            if (errno == EINTR) continue; /* interrupted by signal */
            perror("mq_receive");
            break;
        }
    }

    mq_close(mq);
    mq_unlink(MQ_NAME);
    printf("MQ consumer exiting\n");
    return EXIT_SUCCESS;
}
