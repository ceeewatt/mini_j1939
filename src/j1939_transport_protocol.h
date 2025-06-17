#pragma once

#include "j1939.h"

#include <stdint.h>

enum j1939_tp_connection {
    J1939_TP_CONNECTION_NONE = 0,
    J1939_TP_CONNECTION_BROADCAST,
    J1939_TP_CONNECTION_P2P
};

enum j1939_tp_abort_reason {
    J1939_TP_ABORT_REASON_BUSY = 0,
    J1939_TP_ABORT_REASON_RESOURCES,
    J1939_TP_ABORT_REASON_TIMEOUT,
    J1939_TP_ABORT_REASON_OTHER
};

struct J1939TP {
    // Used for indexing into the global J1939Private array
    int node_idx;

    uint8_t buf[J1939_TP_MAX_PAYLOAD];

    // This is the multi-packet message that our connection is currently forming
    struct J1939Msg msg_info;

    // Used to indicate what kind of connection is currently active (broadcast or peer-to-peer).
    // If this is set to NONE, a connection is not presently active.
    enum j1939_tp_connection connection;

    // Used to indicate whether this node is a sender or receiver in a given connection.
    // True: this node is a sender
    // False: this node is a receiver
    bool sender;

    // This hold the sequence number of the next TP.DT packet.
    // Sender: the next transmitted TP.DT packet will have this sequence number.
    // Receiver: the next received TP.DT packet should have this sequence number.
    uint8_t next_seq;

    // The number of data bytes that remain for the presently open connection.
    // Sender: this number of bytes still needs to be transmitted.
    // Receiver: this number of bytes still needs to be received.
    uint16_t bytes_rem;

    // The total number of Data Transfer packages that need to be transmitted for a given connection.
    uint8_t num_packages;
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

struct __attribute__((packed)) J1939_TP_CM_BAM {
    uint8_t control_byte;
    uint16_t len;
    uint8_t num_packages;
    uint8_t res;
    uint32_t pgn : 24;
};

struct __attribute__((packed)) J1939_TP_CM_ABORT {
    uint8_t control_byte;
    uint8_t abort_reason;
    uint32_t res : 24;
    uint32_t pgn : 24;
};

#define J1939_TP_CM_PGN  (0x00EC00)
#define J1939_TP_CM_LEN  (8)
#define J1939_TP_CM_PRI  (7)

#define J1939_TP_CM_CONTROL_BYTE_RTS  (16)
#define J1939_TP_CM_CONTROL_BYTE_CTS  (17)
#define J1939_TP_CM_CONTROL_BYTE_ACK  (19)
#define J1939_TP_CM_CONTROL_BYTE_BAM  (32)
#define J1939_TP_CM_CONTROL_BYTE_ABORT  (255)

void
j1939_tp_init(
    struct J1939TP* tp,
    int node_idx);

void
j1939_tp_close_connection(
    struct J1939TP* tp);

void
j1939_tp_dispatch(
    struct J1939TP* tp,
    struct J1939Msg* msg);

void
j1939_tp_rx_dt(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);

void
j1939_tp_dt_pack(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);

void
j1939_tp_rx_bam(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam,
    uint8_t msg_src);

void
j1939_tp_bam_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam);

void
j1939_tp_abort_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort,
    enum j1939_tp_abort_reason reason,
    uint32_t pgn);

void
j1939_tp_rx_abort(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort);
