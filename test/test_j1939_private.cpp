#include <catch2/catch_test_macros.hpp>
#include <cstring>

extern "C" {
    struct CanIdConverter {
        uint8_t pri;
        uint8_t dp;
        uint8_t pf;
        uint8_t ps;
        uint8_t sa;
        uint32_t pgn;
    };
    #include "j1939_private.h"
}

TEST_CASE("CAN ID conversion successfully parses fields from 29-bit CAN ID", "[can_id_converter]")
{
    CanIdConverter fields;
    
    SECTION("A PDU Format (PF) field of less than 240 is a destination specific message")
    {
        uint32_t id = 0x18EBF900;
        bool result = can_id_converter(&fields, id);

        REQUIRE(result == false);
        REQUIRE(fields.pri == 6);
        REQUIRE(fields.dp == 0);
        REQUIRE(fields.pf == 0xEB);
        REQUIRE(fields.ps == 0xF9);
        REQUIRE(fields.sa == 0);
        REQUIRE(fields.pgn == 0x00EB00);
    }
    SECTION("A PF field of greater than 240 is a broadcast message")
    {
        uint32_t id = 0x0CF004FE;
        bool result = can_id_converter(&fields, id);

        REQUIRE(result == true);
        REQUIRE(fields.pri == 3);
        REQUIRE(fields.dp == 0);
        REQUIRE(fields.pf == 0xF0);
        REQUIRE(fields.ps == 0x04);
        REQUIRE(fields.sa == 0xFE);
        REQUIRE(fields.pgn == 0x00F004);
    }
    SECTION("The Data Page (DP) bit forms the MSB of the PGN")
    {
        uint32_t id = 0x1DFFABFE;
        bool result = can_id_converter(&fields, id);

        REQUIRE(result == true);
        REQUIRE(fields.pri == 7);
        REQUIRE(fields.dp == 1);
        REQUIRE(fields.pf == 0xFF);
        REQUIRE(fields.ps == 0xAB);
        REQUIRE(fields.sa == 0xFE);
        REQUIRE(fields.pgn == 0x01FFAB);
    }
}

TEST_CASE("CAN frames are successfully converted to J1939 messages", "[can_frame_to_j1939_message]")
{
    J1939 node;
    (void)j1939_init(&node);

    J1939CanFrame frame;
    uint8_t frame_data[8] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };
    frame.len = 8;
    std::memcpy(frame.data, frame_data, 8);

    J1939Msg msg;
    uint8_t msg_data[8];
    msg.data = msg_data;

    SECTION("If bit 31 of the CAN ID is not set, the frame is considered to be a standard CAN 2.0A frame and is rejected")
    {
        // Most signicant nibble: 0x7 = 0b0111
        frame.id = 0x78EBF900;
        bool result = can_frame_to_j1939_message(&node, &frame, &msg);

        REQUIRE(result == false);
    }
    SECTION("J1939Msg struct is filled out correctly (PDU1 format)")
    {
        frame.id = 0x88EBF912;
        bool result = can_frame_to_j1939_message(&node, &frame, &msg);

        REQUIRE(result == true);
        REQUIRE(msg.dst == 0xF9);
        REQUIRE(msg.pgn == 0x00EB00);
        REQUIRE(msg.src == 0x12);
        REQUIRE(msg.pri == 2);
        REQUIRE(msg.len == 8);
        REQUIRE(std::memcmp(msg.data, frame_data, 8) == 0);
    }
    SECTION("J1939Msg struct is filled out correctly (PDU2 format)")
    {
        frame.id = 0x8CF004FE;
        bool result = can_frame_to_j1939_message(&node, &frame, &msg);

        REQUIRE(result == true);
        REQUIRE(msg.dst == 0xFF);
        REQUIRE(msg.pgn == 0x00F004);
        REQUIRE(msg.src == 0xFE);
        REQUIRE(msg.pri == 3);
    }
    SECTION("CAN frames are considered invalid if they're length is greater than 8")
    {
        frame.id = 0x8CF004FE;
        frame.len = 9;
        bool result = can_frame_to_j1939_message(&node, &frame, &msg);

        REQUIRE(result == false);
    }
}
