#include "j1939.h"
#include "j1939_define.h"
#include "j1939_private.h"
#include <string.h>
#include <math.h>

/* File-static functions */

static void
forward_msg(
    struct J1939* node);

static bool
is_connection_active(
    struct J1939* node);

static void
tx_ack(
    struct J1939* node);

static void
tx_abort(
    struct J1939* node,
    enum j1939_tp_cm_abort_reason reason);

static void
tx_cts(
    struct J1939* node);

static void
rx_cm_rts(
    struct J1939* node,
    struct J1939_TP_CM_RTS* rts,
    uint8_t dst);

static void
rx_cm_cts(
    struct J1939* node,
    struct J1939_TP_CM_CTS* cts);

static void
rx_cm_ack(
    struct J1939* node,
    struct J1939_TP_CM_ACK* ack);

static void
rx_cm_abort(
    struct J1939* node,
    struct J1939_TP_CM_ABORT* abort);

static void
tx_bam(
    struct J1939* node);

static void
tx_rts(
    struct J1939* node);

static void
rx_cm_bam(
    struct J1939* node,
    struct J1939_TP_CM_BAM* bam,
    uint8_t src);

static void
tx_dt(
    struct J1939* node);

static void
broadcast_update(
    struct J1939* node);

static void
p2p_update(
    struct J1939* node);

/* Function Definitions */

void
j1939_tp_init(
    struct J1939* node)
{
    node->tp.timer_ms = 0;
    node->tp.connection_type = J1939_TP_NONE;
    node->tp.received_cts = false;
}

void
j1939_tp_close_connection(
    struct J1939* node)
{
    node->tp.connection_type = J1939_TP_NONE;
}

// Attempt to queue a multi-byte message for transmission
// Copy a multi-byte message to an internal buffer, allowing this layer to
//  transmit the TP message non-blocking
bool
j1939_tp_queue(
    struct J1939* node,
    struct J1939Msg* msg)
{
    if (is_connection_active(node)) {
        return false;
    }

    memcpy(node->tp.buf, msg->data, msg->len);
    node->tp.pgn = msg->pgn;
    node->tp.len = msg->len;
    node->tp.dst = msg->dst;
    node->tp.pri = msg->pri;

    node->tp.sender = true;
    node->tp.next_seq = 1;
    node->tp.bytes_rem = msg->len;
    node->tp.num_packets = (uint8_t)ceilf((float)msg->len / 7.0f);
    node->tp.received_cts = false;
    node->tp.timer_ms = 0;

    if (msg->dst == J1939_ADDR_GLOBAL) {
        node->tp.connection_type = J1939_TP_BROADCAST;
        tx_bam(node);
    }
    else {
        node->tp.connection_type = J1939_TP_P2P;
        tx_rts(node);
    }

    return true;
}

void
j1939_tp_update(
    struct J1939* node)
{
    node->tp.timer_ms += node->priv.tick_rate_ms;

    switch (node->tp.connection_type)
    {
        case J1939_TP_BROADCAST:
            broadcast_update(node);
            break;
        case J1939_TP_P2P:
            p2p_update(node);
            break;
        case J1939_TP_NONE:
        default:
            break;
    }
}

// Receive a Connection Management packet
void
j1939_tp_rx_cm(
    struct J1939* node,
    struct J1939Msg* msg)
{
    switch (*msg->data) {
        case J1939_TP_CM_BAM_CONTROL_BYTE:
            rx_cm_bam(node, (struct J1939_TP_CM_BAM*)msg->data, msg->src);
            break;
        case J1939_TP_CM_RTS_CONTROL_BYTE:
            rx_cm_rts(node, (struct J1939_TP_CM_RTS*)msg->data, msg->src);
            break;
        case J1939_TP_CM_CTS_CONTROL_BYTE:
            rx_cm_cts(node, (struct J1939_TP_CM_CTS*)msg->data);
            break;
        case J1939_TP_CM_ACK_CONTROL_BYTE:
            rx_cm_ack(node, (struct J1939_TP_CM_ACK*)msg->data);
            break;
        case J1939_TP_CM_ABORT_CONTROL_BYTE:
            rx_cm_abort(node, (struct J1939_TP_CM_ABORT*)msg->data);
            break;
        default:
            return;
    }
}

// Recieve a Data Transfer packet
void
j1939_tp_rx_dt(
    struct J1939* node,
    struct J1939Msg* msg)
{
    // Sanity check: ensure there's an active connection
    if (!is_connection_active(node)) {
        return;
    }

    struct J1939_TP_DT* dt = (struct J1939_TP_DT*)msg->data;
    uint8_t seq = dt->seq;

    // Ensure we're receiving the expected sequence number
    if (seq != node->tp.next_seq++) {
        tx_abort(node, J1939_REASON_OTHER);
        j1939_tp_close_connection(node);
        return;
    }

    uint8_t* buf = node->tp.buf + ((seq - 1) * 7);
    uint8_t* data = ((uint8_t*)dt) + 1;
    int bytes_rem = (node->tp.bytes_rem < 7 ? node->tp.bytes_rem : 7);

    for (int i = 0; i < bytes_rem; ++i) {
        *(buf + i) = data[i];
        node->tp.bytes_rem--;
    }

    node->tp.timer_ms = 0;
}

/* File-static function definitions */

// Pass the complete multi-byte to its destination
static void
forward_msg(
    struct J1939* node)
{
    struct J1939Msg msg;
    msg.pgn = node->tp.pgn;
    msg.data = node->tp.buf;
    msg.len = node->tp.len;
    msg.src = node->tp.src;
    msg.dst =
        (node->tp.connection_type == J1939_TP_P2P) ?
        node->priv.source_address :
        J1939_ADDR_GLOBAL;
    msg.pri = node->tp.pri;

    dispatch(node, &msg);
}

static bool
is_connection_active(
    struct J1939* node)
{
    return (node->tp.connection_type != J1939_TP_NONE);
}

static void
tx_ack(
    struct J1939* node)
{
    struct J1939_TP_CM_ACK ack;
    ack.control_byte = J1939_TP_CM_ACK_CONTROL_BYTE;
    ack.len = node->tp.len;
    ack.packets = node->tp.num_packets;
    ack.res = 0xFF;
    ack.pgn = node->tp.pgn;

    struct J1939Msg msg;
    msg.pgn = J1939_TP_CM_PGN;
    msg.data = (uint8_t*)&ack;
    msg.len = 8;
    msg.dst = node->tp.dst;
    msg.pri = J1939_TP_CM_DEFAULT_PRIORITY;

    j1939_tx(node, &msg);
}

static void
tx_abort(
    struct J1939* node,
    enum j1939_tp_cm_abort_reason reason)
{
    struct J1939_TP_CM_ABORT abort;
    abort.control_byte = J1939_TP_CM_ABORT_CONTROL_BYTE;
    abort.abort_reason = reason;
    abort.res = 0xFFFFFF;
    abort.pgn = node->tp.pgn;

    struct J1939Msg msg;
    msg.pgn = J1939_TP_CM_PGN;
    msg.data = (uint8_t*)&abort;
    msg.len = 8;
    msg.dst = node->tp.dst;
    msg.pri = J1939_TP_CM_DEFAULT_PRIORITY;

    j1939_tx(node, &msg);
}

static void
tx_cts(
    struct J1939* node)
{
    struct J1939_TP_CM_CTS cts;
    cts.control_byte = J1939_TP_CM_CTS_CONTROL_BYTE;
    cts.packets = J1939_TP_CM_RTS_MAX_PACKETS;
    cts.next_seq = 1;
    cts.res = 0xFFFF;
    cts.pgn = node->tp.pgn;

    struct J1939Msg msg;
    msg.pgn = J1939_TP_CM_PGN;
    msg.len = 8;
    msg.dst = node->tp.dst;
    msg.pri = J1939_TP_CM_DEFAULT_PRIORITY;
    msg.data = (uint8_t*)&cts;

    j1939_tx(node, &msg);
}

static void
rx_cm_rts(
    struct J1939* node,
    struct J1939_TP_CM_RTS* rts,
    uint8_t src)
{
    if (is_connection_active(node)) {
        return;
    }
    
    node->tp.connection_type = J1939_TP_P2P;
    node->tp.sender = false;
    node->tp.bytes_rem = rts->len;
    node->tp.timer_ms = 0;

    node->tp.len = rts->len;
    node->tp.num_packets = rts->packets;
    node->tp.pgn = rts->pgn;
    node->tp.next_seq = 1;
    node->tp.src = src;
    
    tx_cts(node);
}

static void
rx_cm_cts(
    struct J1939* node,
    struct J1939_TP_CM_CTS* cts)
{
    if (node->tp.pgn != cts->pgn ||
        node->tp.timer_ms >= J1939_TP_TIMEOUT_TR)
    {
        tx_abort(node, J1939_REASON_TIMEOUT);
        j1939_tp_close_connection(node);
    }

    node->tp.next_seq = 1;
    node->tp.timer_ms = 0;
    node->tp.received_cts = true;
}

static void
rx_cm_ack(
    struct J1939* node,
    struct J1939_TP_CM_ACK* ack)
{
    if (!is_connection_active(node))
        return;

    j1939_tp_close_connection(node);
}

static void
rx_cm_abort(
    struct J1939* node,
    struct J1939_TP_CM_ABORT* abort)
{
    (void)abort;
    j1939_tp_close_connection(node);
}

static void
tx_bam(
    struct J1939* node)
{
    struct J1939_TP_CM_BAM bam;
    bam.control_byte = J1939_TP_CM_BAM_CONTROL_BYTE;
    bam.len = node->tp.len;
    bam.packets = node->tp.num_packets;
    bam.res = 0xFF;
    bam.pgn = node->tp.pgn;

    struct J1939Msg msg;
    msg.pgn = J1939_TP_CM_PGN;
    msg.data = (uint8_t*)&bam;
    msg.len = 8;
    msg.src = node->priv.source_address;
    msg.dst = node->tp.dst;
    msg.pri = J1939_TP_CM_DEFAULT_PRIORITY;

    j1939_tx(node, &msg);
}

static void
tx_rts(
    struct J1939* node)
{
    struct J1939_TP_CM_RTS rts;
    rts.control_byte = J1939_TP_CM_RTS_CONTROL_BYTE;
    rts.len = node->tp.len;
    rts.packets = node->tp.num_packets;
    rts.max_packets = J1939_TP_CM_RTS_MAX_PACKETS;
    rts.pgn = node->tp.pgn;

    struct J1939Msg msg;
    msg.pgn = J1939_TP_CM_PGN;
    msg.len = 8;
    msg.data = (uint8_t*)&rts;
    msg.dst = node->tp.src;
    msg.pri = J1939_TP_CM_DEFAULT_PRIORITY;

    j1939_tx(node, &msg);
}

static void
rx_cm_bam(
    struct J1939* node,
    struct J1939_TP_CM_BAM* bam,
    uint8_t src)
{
    if (is_connection_active(node))
        return;

    node->tp.connection_type = J1939_TP_BROADCAST;
    node->tp.sender = false;
    node->tp.next_seq = 1;
    node->tp.bytes_rem = bam->len;
    node->tp.num_packets = bam->packets;

    node->tp.pgn = bam->pgn;
    node->tp.len = bam->len;
    node->tp.dst = J1939_ADDR_GLOBAL;
    node->tp.src = src;

    // TODO: pgn lookup to determine priority
    node->tp.pri = J1939_DEFAULT_PRIORITY;
}

static void
tx_dt(
    struct J1939* node)
{
    struct J1939_TP_DT dt;
    dt.seq = node->tp.next_seq;

    uint8_t* data = ((uint8_t*)&dt) + 1;
    uint8_t* buf = node->tp.buf + ((node->tp.next_seq - 1) * 7);

    // Copy at most 7 bytes from the buffer to the DT packet
    // Fill the remaining data bytes with 0xFF
    int i = 0;
    int bytes_rem = (node->tp.bytes_rem < 7) ? node->tp.bytes_rem : 7;
    for (; i < bytes_rem; i++) {
        data[i] = buf[i];
        node->tp.bytes_rem--;
    }
    for (; i < 7; i++) {
        data[i] = 0xFF;
    }

    node->tp.next_seq++;

    struct J1939Msg msg;
    msg.pgn = J1939_TP_DT_PGN;
    msg.src = node->priv.source_address;
    msg.pri = J1939_TP_DT_DEFAULT_PRIORITY;
    msg.dst = node->tp.dst;
    msg.data = (uint8_t*)&dt;
    msg.len = 8;

    j1939_tx(node, &msg);
}

static void
broadcast_update(
    struct J1939* node)
{
    if (node->tp.sender) {
        if (node->tp.timer_ms >= J1939_TP_DT_PERIOD) {
            if (node->tp.bytes_rem) {
                tx_dt(node);
            }
            else {
                j1939_tp_close_connection(node);
            }
        }
    }
    else {
        if (node->tp.bytes_rem == 0)
        {
            // Pass message to application and close connection
            forward_msg(node);
            j1939_tp_close_connection(node);
        }
        else if (node->tp.timer_ms >= J1939_TP_TIMEOUT_T1)
        {
            tx_abort(node, J1939_REASON_TIMEOUT);
            j1939_tp_close_connection(node);
        }
    }
}

static void
p2p_update(
    struct J1939* node)
{
    if (node->tp.sender) {

        // We need to receive a CTS from the receiver before data transfer
        if (node->tp.received_cts) {
            if (node->tp.bytes_rem) {
                if (node->tp.timer_ms >= J1939_TP_DT_PERIOD) {
                    tx_dt(node);
                }
            }
            else {
                if (node->tp.timer_ms >= J1939_TP_TIMEOUT_T3) {
                    // We should have received a end of msg ack after the last data packet
                    tx_abort(node, J1939_REASON_TIMEOUT);
                    j1939_tp_close_connection(node);
                }
            }
        }
        else if (node->tp.timer_ms >= J1939_TP_TIMEOUT_TR)
        {
            tx_abort(node, J1939_REASON_TIMEOUT);
            j1939_tp_close_connection(node);
        }

    }
    else {
        if (!node->tp.bytes_rem) {
            tx_ack(node);
            forward_msg(node);
            j1939_tp_close_connection(node);
        }
        else if (node->tp.timer_ms >= J1939_TP_TIMEOUT_T1) {
            tx_abort(node, J1939_REASON_TIMEOUT);
            j1939_tp_close_connection(node);
        }
    }
}

