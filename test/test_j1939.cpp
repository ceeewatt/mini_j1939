#include "test_j1939.hpp"

#include <iostream>

J1939 TestJ1939::node;
uint8_t TestJ1939::msg_buf[J1939_TP_MAX_PAYLOAD];
J1939Msg TestJ1939::msg { .data = TestJ1939::msg_buf };
J1939Name TestJ1939::name {
    .identity = 0xA,
    .manufacturer = 0xB,
    .ecu_instance = 0,
    .function_instance = 1,
    .function = 2,
    .vehicle_system = 0xC,
    .vehicle_system_instance = 0xD,
    .industry_group = 7,
    .arbitrary_addr_capable = 1
};

void startup_delay_250ms(void* param)
{
    (void)param;
    return;
}

bool TestJ1939::can_rx(J1939CanFrame* jframe)
{
    (void)jframe;
    return false;
}

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

void TestJ1939::j1939_rx(J1939Msg *msg)
{
    TestJ1939::msg.pgn = msg->pgn;
    TestJ1939::msg.len = msg->len;
    TestJ1939::msg.dst = msg->dst;
    TestJ1939::msg.src = msg->src;
    TestJ1939::msg.pri = msg->pri;
    std::memcpy(TestJ1939::msg.data, msg->data, msg->len);
}

void TestJ1939::testRunStarting(Catch::TestRunInfo const& test_run_info)
{
    const uint8_t preferred_address = 0x27;
    const bool init_result =
        j1939_init(
            &TestJ1939::node,
            &TestJ1939::name,
            preferred_address,
            10,
            &TestJ1939::can_rx,
            &TestJ1939::can_tx,
            &TestJ1939::j1939_rx,
            startup_delay_250ms,
            nullptr);

    if (!init_result)
    {
        std::cout << "testRunStarting(): init failure\n";
        exit(1);
    }
}

CATCH_REGISTER_LISTENER(TestJ1939)
