#pragma once

/* ============================================================================
 * File: j1939_transport_protocol.h
 *
 * Description: The transport protocol (TP) procedure allows for transmission of
 *              messages containing more than 8 bytes of payload. The standard
 *              defines a sequence by which the data is transmitted in a series
 *              of messages that each hold 8 bytes of data. When two or more
 *              nodes enter into this sequence, it's a called an open connection.
 *              There are two different types of connections. Broadcast
 *              connections are opened when the multi-packet message is
 *              addressed to the global address (255). Peer-to-peer (p2p)
 *              connections are opened when the multi-packet message is
 *              addressed to a single node. The transport protocol procedure
 *              differs slightly depending on the type of connection. TP data
 *              transfer and connection management are handled via the exchange
 *              of TP.DT and TP.CM PGNs, respectively.
 * ============================================================================
 */

#include "j1939.h"

#include <stdint.h>

/* ============================================================================
 *
 * Section: Macros
 *
 * ============================================================================
 */

// While a connection is open, transmit TP packets at this period (ms)
#define J1939_TP_TX_PERIOD  (50)

// No limit on the number of packages sent during a P2P connection
#define J1939_TP_CM_RTS_MAX_PACKAGES  (0xFF)

/* ============================================================================
 *
 * Section: Type definitions
 *
 * ============================================================================
 */

// A transport protocol connection be either:
//  Broadcast: multi-packet message to sent to global address.
//  Peer-to-peer: multi-packet message is sent to destination-specific address.
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

    // Buffer for holding the TP.DT payload
    uint8_t buf[J1939_TP_MAX_PAYLOAD];

    // This is the multi-packet message that our connection is currently trying
    //  to send or receive. If we're the sender, this struct is basically
    //  copied from the J1939Msg passed to j1939_tp_queue(). If we're the
    //  receiver, we have to derive this information from the BAM or RTS msg.
    //  This msg is passed to j1939_rx() once fully received.
    struct J1939Msg msg_info;

    // If this is set to NONE, a connection is not presently active.
    // Indicates what kind of connection is currently active. If set to NONE, a
    //  connection is not presently active.
    enum j1939_tp_connection connection;

    // True: this node is the sender (and the one that opened the connection).
    // False: this node is the receiver.
    bool sender;

    // This holds the sequence number of the next TP.DT packet.
    // Sender: the next transmitted TP.DT packet will have this sequence number.
    // Receiver: the next received TP.DT packet should have this sequence number.
    uint8_t next_seq;

    // The number of data bytes that remain for the presently open connection.
    // Sender: this number of bytes still needs to be transmitted.
    // Receiver: this number of bytes still needs to be received.
    uint16_t bytes_rem;

    // The number of TP.DT messages that are necessary to transmit the
    //  full multi-packet message.
    uint8_t num_packages;

    // Used for periodic message transmission and timeout tracking
    int timer_ms;
    int tick_rate_ms;

    // Used for P2P connections to signal when we're ready for data transfer.
    // Sender: we've received a CTS msg and can begin transmitting data.
    // Receiver: we've responded to an RTS with a CTS and are ready for data.
    bool clear_to_send;
};

/* ============================================================================
 *
 * Section: Function prototypes
 *
 * ============================================================================
 */

void
j1939_tp_init(
    struct J1939TP* tp,
    int node_idx,
    int tick_rate_ms);

// Attempt to queue up a multi-packet message for transmission. Return true if
//  successful, return false otherwise.
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
