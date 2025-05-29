#include "node_helper.h"

int main(void)
{
    const uint8_t src_addr = NODE1_SRC_ADDR;
    struct J1939 node;

    struct J1939Name name = {
        .identity = NODE1_IDENTITY,
        .manufacturer = 0,
        .ecu_instance = 0,
        .function_instance = 0,
        .function = 0,
        .vehicle_system = 0,
        .vehicle_system_instance = 0,
        .industry_group = 0,
        .arbitrary_addr_capable = 1
    };

    node_init(&node, &name, src_addr);
    node_superloop(&node);

    return 0;
}

void node_tx_1hz(struct J1939* node)
{
    static int counter_1sec = 0;

    struct dummy1 dummy = {
        .a = ++counter_1sec,
        .b = 0xBE,
        .c = 0xEF
    };
    struct J1939Msg msg;

    msg.pgn = DUMMY1_PGN;
    msg.data = (uint8_t*)&dummy;
    msg.len = sizeof(struct dummy1);
    msg.dst = NODE2_SRC_ADDR;
    msg.pri = 7;

    j1939_tx(node, &msg);
}
