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
    // This is used by the receiver.
    // msg_info.src: the source address of the node that opened the connection
    // msg_info.dst:
    //  - for BAM messages: global address (255)
    //  - for P2P messages: our own source address
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

    // Used for periodic message transmission and timeout tracking
    int timer_ms;
    int tick_rate_ms;

    // This allows the TP layer to pass multi-packet messages directly to application
    J1939_MSG_RX j1939_rx;

    // Used for P2P connections
    // Sender: we have received a CTS message from receiver and thus can begin data transfer
    // Receiver: we have recieved an RTS message and replied with a CTS message, thus, we're ready to begin receiving data
    bool clear_to_send;
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

struct __attribute__((packed)) J1939_TP_CM_RTS {
    uint8_t control_byte;
    uint16_t len;
    uint8_t num_packages;
    uint8_t max_packages;
    uint32_t pgn : 24;
};
struct __attribute__((packed)) J1939_TP_CM_CTS {
    uint8_t control_byte;
    uint8_t num_packages;
    uint8_t next_seq;
    uint16_t res;
    uint32_t pgn : 24;
};
struct __attribute__((packed)) J1939_TP_CM_ACK {
    uint8_t control_byte;
    uint16_t len;
    uint8_t num_packages;
    uint8_t res;
    uint32_t pgn : 24;
};
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

#define J1939_TP_TIMEOUT_TR  (200)
#define J1939_TP_TIMEOUT_TH  (500)
#define J1939_TP_TIMEOUT_T1  (750)
#define J1939_TP_TIMEOUT_T2  (1250)
#define J1939_TP_TIMEOUT_T3  (1250)
#define J1939_TP_TIMEOUT_T4  (1050)

// While a connection is open, transmit TP packets at this period (ms)
#define J1939_TP_TX_PERIOD  (50)

// No limit on the number of packages sent during a P2P connection
#define J1939_TP_CM_RTS_MAX_PACKAGES  (0xFF)

void
j1939_tp_init(
    struct J1939TP* tp,
    int node_idx,
    int tick_rate_ms,
    J1939_MSG_RX j1939_rx);

bool
j1939_tp_queue(
    struct J1939TP* tp,
    struct J1939Msg* msg);

void
j1939_tp_update(
    struct J1939TP* tp);

void
j1939_tp_close_connection(
    struct J1939TP* tp);

void
j1939_tp_dispatch(
    struct J1939TP* tp,
    struct J1939Msg* msg);
