extern "C" {
    #include "j1939_transport_protocol_helper.h"
}

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Data from TP.DT packets are received and added to the buffer", "[j1939_tp_rx_dt]")
{
    J1939TP tp;
    J1939_TP_DT dt;

    SECTION("Sequence numbers must be received in the expected order")
    {
        tp.next_seq = 1;
        tp.bytes_rem = 7;
        dt.seq = 2;

        bool result = j1939_tp_rx_dt(&tp, &dt);

        // The function should return early and thus skip incrementing next_seq
        REQUIRE(result == false);
        REQUIRE(tp.next_seq == 1);
    }
    SECTION("The correct number of bytes are copied, depending on the number of bytes remaining")
    {
        tp.next_seq = 1;
        tp.bytes_rem = 8;
        std::memset(tp.buf, 0, 9);

        dt.seq = 1;
        uint8_t data1[7] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
        uint8_t data2[7] = { 0x77, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

        std::memcpy(&dt.data0, data1, sizeof(data1));
        (void)j1939_tp_rx_dt(&tp, &dt);

        REQUIRE(tp.bytes_rem == 1);
        REQUIRE(tp.next_seq == 2);
        REQUIRE(std::memcmp(&tp.buf[0], data1, 7) == 0);

        dt.seq = 2;
        std::memcpy(&dt.data0, data2, sizeof(data2));
        j1939_tp_rx_dt(&tp, &dt);

        REQUIRE(tp.bytes_rem == 0);
        REQUIRE(tp.next_seq == 3);
        REQUIRE(tp.buf[7] == 0x77);
        REQUIRE(tp.buf[8] == 0x00);
    }
    SECTION("Data is copied into the correct buffer location, depending on the packet's sequence number")
    {
        tp.next_seq = 255;
        tp.bytes_rem = 7;
        std::memset(tp.buf, 0, sizeof(tp.buf));

        dt.seq = 255;
        uint8_t data[7] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xBE, 0xEE };
        std::memcpy(&dt.data0, data, sizeof(data));

        (void)j1939_tp_rx_dt(&tp, &dt);

        REQUIRE(tp.bytes_rem == 0);
        REQUIRE(std::memcmp(&tp.buf[1778], data, sizeof(data)) == 0);
    }
}

TEST_CASE("Data in the buffer is packed properly into TP.DT packets", "[j1939_tp_dt_pack]")
{
    J1939TP tp;
    J1939_TP_DT dt;

    SECTION("Data is copied from the correct location in the buffer, depending on the sequence number")
    {
        std::memset(tp.buf, 0, sizeof(tp.buf));
        std::memset(&dt, 0, sizeof(dt));

        // Pick a random sequence number and ensure data is copied from the expected locations in the buffer
        tp.next_seq = 18;
        tp.bytes_rem = 7;

        int buf_idx = (tp.next_seq - 1) * 7;
        uint8_t data[7] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xBA, 0xBE, 0xEE };
        std::memcpy(&tp.buf[buf_idx], data, sizeof(data));

        j1939_tp_dt_pack(&tp, &dt);
        
        REQUIRE(dt.seq == 18);
        REQUIRE(tp.next_seq == 19);
        REQUIRE(std::memcmp(&dt.data0, data, sizeof(data)) == 0);
    }
    SECTION("All unused bytes in the last packet are set of 0xFF")
    {
        tp.next_seq = 1;
        tp.bytes_rem = 4;

        uint8_t data[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
        uint8_t expected[7] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0xFF, 0xFF };

        std::memcpy(tp.buf, data, sizeof(data));

        j1939_tp_dt_pack(&tp, &dt);

        REQUIRE(std::memcmp(&dt.data0, expected, sizeof(expected)) == 0);
    }
}
