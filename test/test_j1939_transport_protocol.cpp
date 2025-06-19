#include "test_j1939.hpp"

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

TEST_CASE("Opening a connection results in the sender transmitting the appropriate TP message", "[j1939_tp_queue]")
{
    J1939Private* jp = &g_j1939[TestJ1939::node.node_idx];
    J1939Msg msg;

    SECTION("Sender cannot open a new connection while one is already active")
    {
        jp->tp.connection = J1939_TP_CONNECTION_BROADCAST;
        REQUIRE(j1939_tp_queue(&jp->tp, &msg) == false);

        jp->tp.connection = J1939_TP_CONNECTION_P2P;
        REQUIRE(j1939_tp_queue(&jp->tp, &msg) == false);
    }
    SECTION("Broadcast message results in BAM message transmission")
    {
        uint8_t data[15] = {
            0x5a, 0x62, 0x38, 0xd8, 0x03, 0xd1, 0x40,
            0x73, 0xc5, 0xa7, 0xc7, 0x83, 0x04, 0xd2,
            0x88
        };
        msg.pgn = 0xABCD;
        msg.data = data;
        msg.len = sizeof(data);
        msg.dst = J1939_ADDR_GLOBAL;
        msg.pri = J1939_DEFAULT_PRIORITY;

        j1939_tp_close_connection(&jp->tp);
        bool result = j1939_tp_queue(&jp->tp, &msg);

        REQUIRE(result == true);
        REQUIRE(std::memcmp(jp->tp.buf, data, sizeof(data)) == 0);
        REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_BROADCAST);

        REQUIRE(TestJ1939::msg.pgn == J1939_TP_CM_PGN);
        REQUIRE(TestJ1939::msg.len == J1939_TP_CM_LEN);
        REQUIRE(TestJ1939::msg.pri == J1939_TP_CM_PRI);
        REQUIRE(TestJ1939::msg.dst == J1939_ADDR_GLOBAL);
        REQUIRE(TestJ1939::msg.src == jp->j1939_public->source_address);

        J1939_TP_CM_BAM* bam = (J1939_TP_CM_BAM*)TestJ1939::msg.data;

        REQUIRE(bam->control_byte == J1939_TP_CM_CONTROL_BYTE_BAM);
        REQUIRE(bam->len == sizeof(data));
        REQUIRE(bam->num_packages == 3);
        REQUIRE(bam->pgn == 0xABCD);
    }
}
