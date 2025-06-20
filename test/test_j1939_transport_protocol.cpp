#include "test_j1939.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cmath>

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

TEST_CASE("Broadcast sender", "[j1939_tp_queue][j1939_tp_broadcast_update_sender]")
{
    J1939Private* jp = &g_j1939[TestJ1939::node.node_idx];
    J1939Msg msg;

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

    // Send BAM
    j1939_tp_close_connection(&jp->tp);
    bool result = j1939_tp_queue(&jp->tp, &msg);

    REQUIRE(result == true);
    REQUIRE(std::memcmp(jp->tp.buf, data, sizeof(data)) == 0);
    REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_BROADCAST);
    REQUIRE(jp->tp.bytes_rem == 15);
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

    // Send first TP.DT packet
    jp->tp.timer_ms = J1939_TP_TX_PERIOD;
    j1939_tp_broadcast_update_sender(&jp->tp);
    J1939_TP_DT* dt = (J1939_TP_DT*)TestJ1939::msg.data;

    REQUIRE(TestJ1939::msg.pgn == J1939_TP_DT_PGN);
    REQUIRE(TestJ1939::msg.len == J1939_TP_DT_LEN);
    REQUIRE(TestJ1939::msg.pri == J1939_TP_DT_PRI);
    REQUIRE(TestJ1939::msg.dst == J1939_ADDR_GLOBAL);
    REQUIRE(TestJ1939::msg.src == jp->j1939_public->source_address);
    REQUIRE(dt->seq == 1);
    REQUIRE(std::memcmp(&dt->data0, &data[0], 7) == 0);
    REQUIRE(jp->tp.bytes_rem == 8);

    // Send second TP.DT packet
    jp->tp.timer_ms = J1939_TP_TX_PERIOD;
    j1939_tp_broadcast_update_sender(&jp->tp);

    REQUIRE(dt->seq == 2);
    REQUIRE(std::memcmp(&dt->data0, &data[7], 7) == 0);
    REQUIRE(jp->tp.bytes_rem == 1);

    // Send third TP.DT packet
    jp->tp.timer_ms = J1939_TP_TX_PERIOD;
    j1939_tp_broadcast_update_sender(&jp->tp);

    REQUIRE(dt->seq == 3);
    REQUIRE(dt->data0 == data[14]);
    REQUIRE(jp->tp.bytes_rem == 0);

    // Connection should now close
    j1939_tp_broadcast_update_sender(&jp->tp);
    REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_NONE);
}

TEST_CASE("Broadcast reciever", "[j1939_tp_broadcast_update_receiver]")
{
    // Emulate receiving the multi-byte packet from a sender node

    constexpr uint8_t sender_node_address = 0x99;
    constexpr uint32_t msg_pgn = 0xABCD;

    J1939Private* jp = &g_j1939[TestJ1939::node.node_idx];
    j1939_tp_close_connection(&jp->tp);

    uint8_t msg_data[15] = {
        0x5a, 0x62, 0x38, 0xd8, 0x03, 0xd1, 0x40,
        0x73, 0xc5, 0xa7, 0xc7, 0x83, 0x04, 0xd2,
        0x88
    };
    J1939Msg msg { 
        .pgn = msg_pgn,
        .data = msg_data,
        .len = sizeof(msg_data),
        .src = sender_node_address,
        .dst = J1939_ADDR_GLOBAL,
        .pri = J1939_DEFAULT_PRIORITY
    };

    J1939_TP_CM_BAM bam {
        .control_byte = J1939_TP_CM_CONTROL_BYTE_BAM,
        .len = sizeof(msg_data),
        .num_packages = 3,
        .res = 0xFF,
        .pgn = msg_pgn
    };
    J1939Msg bam_msg {
        .pgn = J1939_TP_CM_PGN,
        .data = (uint8_t*)&bam,
        .len = J1939_TP_CM_LEN,
        .src = sender_node_address,
        .dst = J1939_ADDR_GLOBAL,
        .pri = J1939_TP_CM_PRI
    };

    // Emulate receiving the BAM message
    j1939_tp_dispatch(&jp->tp, &bam_msg);

    SECTION("Timeout")
    {
        jp->tp.timer_ms = J1939_TP_TIMEOUT_T1;
        j1939_tp_broadcast_update_receiver(&jp->tp);

        REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_NONE);
    }
    SECTION("Normal data transfer")
    {
        REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_BROADCAST);
        REQUIRE(jp->tp.sender == false);
        REQUIRE(jp->tp.next_seq == 1);
        REQUIRE(jp->tp.bytes_rem == sizeof(msg_data));
        REQUIRE(jp->tp.num_packages == 3);

        J1939Msg dt_msg {
            .pgn = J1939_TP_DT_PGN,
            .data = nullptr,
            .len = J1939_TP_DT_LEN,
            .src = sender_node_address,
            .dst = J1939_ADDR_GLOBAL,
            .pri = J1939_TP_DT_PRI
        };

        jp->tp.timer_ms = 0;

        // Emulate receiving the first TP.DT packet
        J1939_TP_DT dt { .seq = 1 };
        std::memcpy(&dt.data0, msg_data, 7);
        dt_msg.data = (uint8_t*)&dt;
        j1939_tp_dispatch(&jp->tp, &dt_msg);

        // Emulate receiving the second TP.DT packet
        dt.seq = 2;
        std::memcpy(&dt.data0, &msg_data[7], 7);
        j1939_tp_dispatch(&jp->tp, &dt_msg);
        
        // Emulate receiving the third TP.DT packet
        dt.seq = 3;
        dt.data0 = msg_data[14];
        j1939_tp_dispatch(&jp->tp, &dt_msg);

        j1939_tp_broadcast_update_receiver(&jp->tp);

        REQUIRE(jp->tp.bytes_rem == 0);
        REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_NONE);
        REQUIRE(jp->tp.msg_info.pgn == msg_pgn);
        REQUIRE(jp->tp.msg_info.dst == J1939_ADDR_GLOBAL);
        REQUIRE(jp->tp.msg_info.len == sizeof(msg_data));
        REQUIRE(jp->tp.msg_info.src == sender_node_address);
        REQUIRE(jp->tp.msg_info.pri == J1939_DEFAULT_PRIORITY);
        REQUIRE(std::memcmp(jp->tp.buf, TestJ1939::msg.data, sizeof(msg_data)) == 0);
    }
}

TEST_CASE("Peer-to-peer sender", "[j1939_tp_p2p_update_sender]")
{
    J1939Private* jp = &g_j1939[TestJ1939::node.node_idx];

    constexpr uint8_t receiver_node_address = 0x99;
    constexpr uint32_t msg_pgn = 0xABCD;
    const uint8_t sender_node_address = jp->j1939_public->source_address;

    j1939_tp_close_connection(&jp->tp);
    jp->tp.timer_ms = 0;

    uint8_t msg_data[15] = {
        0x5a, 0x62, 0x38, 0xd8, 0x03, 0xd1, 0x40,
        0x73, 0xc5, 0xa7, 0xc7, 0x83, 0x04, 0xd2,
        0x88
    };
    const uint16_t msg_len = sizeof(msg_data);
    const uint8_t msg_num_packages = static_cast<uint8_t>(std::ceil((double)msg_len / 7));
    
    J1939Msg msg { 
        .pgn = msg_pgn,
        .data = msg_data,
        .len = msg_len,
        .src = sender_node_address,
        .dst = receiver_node_address,
        .pri = J1939_DEFAULT_PRIORITY
    };

    // Send RTS
    bool queue_result = j1939_tp_queue(&jp->tp, &msg);

    SECTION("Normal data transfer")
    {
        REQUIRE(queue_result == true);
        REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_P2P);
        REQUIRE(jp->tp.clear_to_send == false);
        REQUIRE(TestJ1939::msg.pgn == J1939_TP_CM_PGN);
        REQUIRE(TestJ1939::msg.len == J1939_TP_CM_LEN);
        REQUIRE(TestJ1939::msg.pri == J1939_TP_CM_PRI);
        REQUIRE(TestJ1939::msg.dst == receiver_node_address);
        REQUIRE(TestJ1939::msg.src == sender_node_address);

        J1939_TP_CM_RTS* rts = (J1939_TP_CM_RTS*)TestJ1939::msg.data;
        REQUIRE(rts->control_byte == J1939_TP_CM_CONTROL_BYTE_RTS);
        REQUIRE(rts->len == msg_len);
        REQUIRE(rts->num_packages == msg_num_packages);
        REQUIRE(rts->max_packages == J1939_TP_CM_RTS_MAX_PACKAGES);
        REQUIRE(rts->pgn == msg_pgn);

        // Emulate receiving a CTS in response
        J1939_TP_CM_CTS cts = {
            .control_byte = J1939_TP_CM_CONTROL_BYTE_CTS,
            .num_packages = msg_num_packages,
            .next_seq = 1,
            .res = 0xFFFF,
            .pgn = msg_pgn
        };
        J1939Msg response_msg {
            .pgn = J1939_TP_CM_PGN,
            .data = (uint8_t*)&cts,
            .len = J1939_TP_CM_LEN,
            .src = receiver_node_address,
            .dst = sender_node_address,
            .pri = J1939_TP_CM_PRI
        };

        j1939_tp_dispatch(&jp->tp, &response_msg);
        REQUIRE(jp->tp.clear_to_send == true);

        // Send all TP.DT packets
        uint8_t bytes_rem_expected = msg_len;
        for (int package = 0; package < msg_num_packages; package++)
        {
            jp->tp.timer_ms = J1939_TP_TX_PERIOD;
            j1939_tp_p2p_update_sender(&jp->tp);

            bytes_rem_expected -= (bytes_rem_expected < 7) ? bytes_rem_expected : 7;
            REQUIRE(jp->tp.bytes_rem == bytes_rem_expected);
        }

        SECTION("Connection closes after receiving end of msg ACK")
        {
            J1939_TP_CM_ACK ack {
                .control_byte = J1939_TP_CM_CONTROL_BYTE_ACK,
                .len = msg_len,
                .num_packages = msg_num_packages,
                .res = 0xFF,
                .pgn = msg_pgn
            };
            response_msg.pgn = J1939_TP_CM_PGN;
            response_msg.data = (uint8_t*)&ack;
            response_msg.len = J1939_TP_CM_LEN;
            response_msg.src = receiver_node_address;
            response_msg.dst = sender_node_address;
            response_msg.pri = J1939_TP_CM_PRI;

            j1939_tp_dispatch(&jp->tp, &response_msg);
            REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_NONE);
        }
        SECTION("Timeout occurs if we don't receive an ACK")
        {
            jp->tp.timer_ms = J1939_TP_TIMEOUT_T3;
            j1939_tp_p2p_update_sender(&jp->tp);

            REQUIRE(TestJ1939::msg.pgn == J1939_TP_CM_PGN);
            REQUIRE(TestJ1939::msg.len == J1939_TP_CM_LEN);
            REQUIRE(TestJ1939::msg.src == sender_node_address);
            REQUIRE(TestJ1939::msg.dst == receiver_node_address);
            REQUIRE(TestJ1939::msg.pri == J1939_TP_CM_PRI);

            J1939_TP_CM_ABORT* abort = (J1939_TP_CM_ABORT*)TestJ1939::msg.data;
            REQUIRE(abort->control_byte == J1939_TP_CM_CONTROL_BYTE_ABORT);
            REQUIRE(abort->abort_reason == J1939_TP_ABORT_REASON_TIMEOUT);
            REQUIRE(abort->pgn == msg_pgn);
            REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_NONE);
        }

    }
    SECTION("Timeout occurs if sender doesn't receive a CTS in response")
    {
        jp->tp.timer_ms = J1939_TP_TIMEOUT_TR;
        j1939_tp_p2p_update_sender(&jp->tp);

        REQUIRE(TestJ1939::msg.pgn == J1939_TP_CM_PGN);
        REQUIRE(TestJ1939::msg.len == J1939_TP_CM_LEN);
        REQUIRE(TestJ1939::msg.src == sender_node_address);
        REQUIRE(TestJ1939::msg.dst == receiver_node_address);
        REQUIRE(TestJ1939::msg.pri == J1939_TP_CM_PRI);

        J1939_TP_CM_ABORT* abort = (J1939_TP_CM_ABORT*)TestJ1939::msg.data;
        REQUIRE(abort->control_byte == J1939_TP_CM_CONTROL_BYTE_ABORT);
        REQUIRE(abort->abort_reason == J1939_TP_ABORT_REASON_TIMEOUT);
        REQUIRE(abort->pgn == msg_pgn);
        REQUIRE(jp->tp.connection == J1939_TP_CONNECTION_NONE);
    }
}

TEST_CASE("Peer-to-peer receiver", "[j1939_tp_p2p_update_receiver]")
{
    J1939TP* tp = &g_j1939[TestJ1939::node.node_idx].tp;

    constexpr uint8_t sender_node_address = 0x99;
    constexpr uint32_t msg_pgn = 0xABCD;
    const uint8_t receiver_node_address = g_j1939[TestJ1939::node.node_idx].j1939_public->source_address;

    j1939_tp_close_connection(tp);
    tp->timer_ms = 0;

    uint8_t msg_data[15] = {
        0x5a, 0x62, 0x38, 0xd8, 0x03, 0xd1, 0x40,
        0x73, 0xc5, 0xa7, 0xc7, 0x83, 0x04, 0xd2,
        0x88
    };
    const uint16_t msg_len = sizeof(msg_data);
    const uint8_t msg_num_packages = static_cast<uint8_t>(std::ceil((double)msg_len / 7));
    
    J1939Msg msg { 
        .pgn = msg_pgn,
        .data = msg_data,
        .len = msg_len,
        .src = sender_node_address,
        .dst = receiver_node_address,
        .pri = J1939_DEFAULT_PRIORITY
    };

    // Emulate receiving an RTS
    J1939_TP_CM_RTS rts {
        .control_byte = J1939_TP_CM_CONTROL_BYTE_RTS,
        .len = msg_len,
        .num_packages = msg_num_packages,
        .max_packages = J1939_TP_CM_RTS_MAX_PACKAGES,
        .pgn = msg_pgn
    };
    J1939Msg received_msg {
        .pgn = J1939_TP_CM_PGN,
        .data = (uint8_t*)&rts,
        .len = J1939_TP_CM_LEN,
        .src = sender_node_address,
        .dst = receiver_node_address,
        .pri = J1939_TP_CM_PRI
    };

    j1939_tp_dispatch(tp, &received_msg);
    REQUIRE(tp->clear_to_send == false);

    // At the specified TX period, we should respond with a CTS
    tp->timer_ms = J1939_TP_TX_PERIOD;
    j1939_tp_p2p_update_receiver(tp);

    REQUIRE(tp->clear_to_send == true);
    REQUIRE(TestJ1939::msg.pgn == J1939_TP_CM_PGN);
    REQUIRE(TestJ1939::msg.len == J1939_TP_CM_LEN);
    REQUIRE(TestJ1939::msg.src == receiver_node_address);
    REQUIRE(TestJ1939::msg.dst == sender_node_address);
    REQUIRE(TestJ1939::msg.pri == J1939_TP_CM_PRI);

    J1939_TP_CM_CTS* cts = (J1939_TP_CM_CTS*)TestJ1939::msg.data;
    REQUIRE(cts->control_byte == J1939_TP_CM_CONTROL_BYTE_CTS);
    REQUIRE(cts->num_packages == msg_num_packages);
    REQUIRE(cts->next_seq == 1);
    REQUIRE(cts->res == 0xFFFF);
    REQUIRE(cts->pgn == msg_pgn);

    SECTION("Timeout if we don't receive data packets at given frequency")
    {
        tp->timer_ms = J1939_TP_TIMEOUT_T1;
        j1939_tp_p2p_update_receiver(tp);

        REQUIRE(tp->connection == J1939_TP_CONNECTION_NONE);
    }
    SECTION("Send ACK and forward multi-packet msg if received successfully")
    {
        // Emulate receiving each of the TP.DT packets
        J1939_TP_DT dt { .seq = 1 };
        received_msg.pgn = J1939_TP_DT_PGN;
        received_msg.data = (uint8_t*)&dt;
        received_msg.len = J1939_TP_DT_LEN;
        received_msg.src = sender_node_address;
        received_msg.dst = receiver_node_address;
        received_msg.pri = J1939_TP_DT_PRI;

        for (int i = 0; i < msg_num_packages; ++i)
        {
            std::memcpy(
                &dt.data0,
                &msg_data[(tp->next_seq - 1) * 7],
                (tp->bytes_rem < 7) ? tp->bytes_rem : 7);
            j1939_tp_dispatch(tp, &received_msg);

            dt.seq++;
        }
        j1939_tp_p2p_update_receiver(tp);

        REQUIRE(TestJ1939::msg.pgn == msg_pgn);
        REQUIRE(TestJ1939::msg.len == msg_len);
        REQUIRE(TestJ1939::msg.src == sender_node_address);
        REQUIRE(TestJ1939::msg.dst == receiver_node_address);
        REQUIRE(TestJ1939::msg.pri == J1939_DEFAULT_PRIORITY);
        REQUIRE(std::memcmp(TestJ1939::msg.data, msg_data, sizeof(msg_data)) == 0);
        REQUIRE(tp->connection == J1939_TP_CONNECTION_NONE);
    }
}
