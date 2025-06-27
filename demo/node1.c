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

    if (!node_init(&node, &name, src_addr))
        return -1;

    node_superloop_oneshot(&node);

    return 0;
}

void node_tx_1hz(struct J1939* node)
{
    struct test_long test;
    struct J1939Msg msg;

    msg.pgn = TEST_LONG_PGN;
    msg.data = (uint8_t*)&test;
    msg.len = TEST_LONG_LEN;
    msg.dst = 0xFF;
    msg.pri = TEST_LONG_PRI;

    j1939_tx(node, &msg);
}

void node_tx_oneshot(struct J1939* node)
{
    struct test_long test = {
        .zero = 0x00,
        .one = 0x11,
        .two = 0x22,
        .three = 0x33,
        .four = 0x44,
        .five = 0x55,
        .six = 0x66,
        .seven = 0x77,
        .eight = 0x88
    };
    struct J1939Msg msg;

    msg.pgn = TEST_LONG_PGN;
    msg.data = (uint8_t*)&test;
    msg.len = TEST_LONG_LEN;
    msg.dst = 0xFF;
    msg.pri = TEST_LONG_PRI;

    j1939_tx(node, &msg);
}
