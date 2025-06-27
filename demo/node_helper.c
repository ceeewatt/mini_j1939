#include "node_helper.h"
#include "j1939_app.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

struct J1939* g_node;
static const int tick_rate_ms = 10;
static int counter_10ms = 0;

static void timer_handler(int signum)
{
    (void)signum;
    counter_10ms++;
    j1939_update(g_node);
}

static bool start_timer()
{
    struct sigaction sa;
    sa.sa_handler = timer_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = tick_rate_ms * 1000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = tick_rate_ms * 1000;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return false;
    }

    return true;
}

bool node_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t src_addr)
{
    const char* device = "vcan0";

    g_node = node;

    j1939_app_init(node, name, src_addr, tick_rate_ms, device);
    return start_timer();
}

void node_superloop(
    struct J1939* node)
{
    printf("Press enter to start\n");
    getchar();
    printf("Starting...\n\n");
    printf("ID        P  SRC   DST   LEN DATA\n");

    while (true)
    {
        if ((counter_10ms % 100) == 0)
            node_tx_1hz(node);
    }
}

void node_superloop_oneshot(struct J1939* node)
{
    printf("Press any key to send packet ");
    while (true)
    {
        if (getchar() != EOF)
        {
            printf("sending...\n\n");
            node_tx_oneshot(node);
            printf("Press any key to send packet ");
        }
        else
        {
            continue;
        }
    }
}
