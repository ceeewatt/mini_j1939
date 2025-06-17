#pragma once

#include "j1939_define.h"

#include <stdint.h>

struct J1939TP {
    uint8_t buf[J1939_TP_MAX_PAYLOAD];

    // This hold the sequence number of the next TP.DT packet.
    // Sender: the next transmitted TP.DT packet will have this sequence number.
    // Receiver: the next received TP.DT packet should have this sequence number.
    uint8_t next_seq;

    // The number of data bytes that remain for the presently open connection.
    // Sender: this number of bytes still needs to be transmitted.
    // Receiver: this number of bytes still needs to be received.
    uint16_t bytes_rem;
};

struct __attribute__((packed)) J1939_TP_DT {
    uint8_t seq;
    uint8_t data0;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
    uint8_t data6;
    uint8_t data7;
};
#define J1939_TP_DT_PGN  (0x00EB00)
#define J1939_TP_DT_LEN  (8)
#define J1939_TP_DT_PRI  (7)

void
j1939_tp_rx_dt(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);

void
j1939_tp_dt_pack(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);
