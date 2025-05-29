#include "node_helper.h"
#include "j1939_app.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

void node_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t src_addr)
{
    const char* device = "vcan0";
    const int tick_rate_ms = 10;

    j1939_app_init(node, name, src_addr, device, tick_rate_ms);
}

void node_superloop(
    struct J1939* node)
{
    const int tick_rate_ms = 10;

    int counter_10ms = 0;

    printf("Press enter to start\n");
    getchar();
    printf("Starting...\n");

    while (true)
    {
        usleep(tick_rate_ms * 1000);

        if ((++counter_10ms % 100) == 0)
            node_tx_1hz(node);

        j1939_update(node);
    }
}
