#include "test_j1939.hpp"

J1939 TestJ1939::node;
uint8_t TestJ1939::msg_buf[J1939_TP_MAX_PAYLOAD];
J1939Msg TestJ1939::msg { .data = TestJ1939::msg_buf };

#include <iostream>
bool TestJ1939::can_tx(J1939Msg* msg)
{
    TestJ1939::msg.pgn = msg->pgn;
    TestJ1939::msg.len = msg->len;
    TestJ1939::msg.dst = msg->dst;
    TestJ1939::msg.src = msg->src;
    TestJ1939::msg.pri = msg->pri;
    std::memcpy(TestJ1939::msg.data, msg->data, msg->len);

    return true;
}

void TestJ1939::testRunStarting(Catch::TestRunInfo const& test_run_info)
{
    const uint8_t preferred_address = 0x27;
    (void)j1939_init(
        &TestJ1939::node,
        preferred_address,
        0,
        NULL,
        &TestJ1939::can_tx,
        NULL);
}

CATCH_REGISTER_LISTENER(TestJ1939)
