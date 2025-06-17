#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <iostream>

extern "C" {
    #include "j1939.h"

    void inspect_can_frame(
        struct J1939* node,
        struct J1939Msg* msg,
        struct J1939CanFrame* frame);
}

void app_rx(J1939Msg* msg)
{
    (void)msg;
}


TEST_CASE("Test inspect_can_frame()") {
    J1939 node { 0 };
    J1939Msg msg { 0 };
    J1939CanFrame frame { 0 };

    node.priv.app_rx = app_rx;

    SECTION("CAN 2.0A messages are discarded") {
        const J1939Msg msg_comp { 0 };

        // Function should return early if bit 31 of the CAN ID is not set
        frame.can_id = ~(1 << 31);
        inspect_can_frame(&node, &msg, &frame);

        REQUIRE(std::memcmp(&msg, &msg_comp, sizeof(J1939Msg)) == 0);
    }
    SECTION("Destination-specific messages are discarded if not addressed to us") {
        const J1939Msg msg_comp { 0 };

        // PS = 4
        frame.can_id = 0xCEF04FE;
        inspect_can_frame(&node, &msg, &frame);

        REQUIRE(std::memcmp(&msg, &msg_comp, sizeof(J1939Msg)) == 0);
    }
    SECTION("Destination-specific messages addressed to use are kept") {
        uint8_t data_comp[4] { 0x12, 0x23, 0x34, 0x45 };
        const J1939Msg msg_comp {
            .pgn = 0xEF00,
            .data = data_comp,
            .len = 4,
            .src = 0xFE,
            .dst = 4,
            .pri = 3
        };

        uint8_t msg_buf[4] { 0x12, 0x23, 0x34, 0x45 };
        msg.data = msg_buf;

        // PS = 4
        frame.can_id = 0x8CEF04FE;
        frame.len = 4;
        frame.data = data_comp;
        node.priv.source_address = 4;
        inspect_can_frame(&node, &msg, &frame);

        REQUIRE(msg.pgn == msg_comp.pgn);
        REQUIRE(msg.len == msg_comp.len);
        REQUIRE(msg.src == msg_comp.src);
        REQUIRE(msg.dst == msg_comp.dst);
        REQUIRE(msg.pri == msg_comp.pri);
        REQUIRE(std::memcmp(msg.data, msg_comp.data, 4) == 0);
    }
    SECTION("Broadcast messages are kept") {
        uint8_t data_comp[4] { 0x12, 0x23, 0x34, 0x45 };
        const J1939Msg msg_comp {
            .pgn = 0xF004,
            .data = data_comp,
            .len = 4,
            .src = 0xFE,
            .dst = J1939_ADDR_GLOBAL,
            .pri = 3
        };

        uint8_t msg_buf[4] { 0x12, 0x23, 0x34, 0x45 };
        msg.data = msg_buf;

        // PS = 4
        frame.can_id = 0x8CF004FE;
        frame.len = 4;
        frame.data = data_comp;
        node.priv.source_address = 4;
        inspect_can_frame(&node, &msg, &frame);

        REQUIRE(msg.pgn == msg_comp.pgn);
        REQUIRE(msg.len == msg_comp.len);
        REQUIRE(msg.src == msg_comp.src);
        REQUIRE(msg.dst == msg_comp.dst);
        REQUIRE(msg.pri == msg_comp.pri);
        REQUIRE(std::memcmp(msg.data, msg_comp.data, 4) == 0);
    }
}
