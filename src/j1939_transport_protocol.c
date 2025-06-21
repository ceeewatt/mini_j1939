#include "j1939_transport_protocol_helper.h"
#include "j1939_private.h"

#include <string.h>

/* ============================================================================
 *
 * Section: Macros
 *
 * ============================================================================
 */

// Find the ceiling of the result of a / b, where a and b are positive integers
#define CEIL_DIV(a, b)  ( ((a) / (b)) + (((a) % (b)) != 0) )

/* ============================================================================
 *
 * Section: Static function prototypes
 *
 * ============================================================================
 */

static bool
is_connection_active(
    struct J1939TP* tp);

/* ============================================================================
 *
 * Section: Function definitions
 *
 * ============================================================================
 */

void
j1939_tp_init(
    struct J1939TP* tp,
    int node_idx,
    int tick_rate_ms)
{
    tp->node_idx = node_idx;

    tp->connection = J1939_TP_CONNECTION_NONE;
    tp->msg_info.data = tp->buf;
    tp->tick_rate_ms = tick_rate_ms;
}

bool
j1939_tp_queue(
    struct J1939TP* tp,
    struct J1939Msg* msg)
{
    if (is_connection_active(tp))
        return false;

    memcpy(tp->buf, msg->data, msg->len);
    tp->sender = true;
    tp->next_seq = 1;
    tp->bytes_rem = msg->len;
    tp->num_packages = CEIL_DIV(msg->len, 7);
    tp->timer_ms = 0;
    tp->clear_to_send = false;

    tp->msg_info.pgn = msg->pgn;
    tp->msg_info.len = msg->len;
    tp->msg_info.src = msg->src;
    tp->msg_info.dst = msg->dst;
    tp->msg_info.pri = msg->pri;

    if (msg->dst == J1939_ADDR_GLOBAL)
    {
        tp->connection = J1939_TP_CONNECTION_BROADCAST;

        struct J1939_TP_CM_BAM bam;
        j1939_tp_bam_pack(tp, &bam);
        j1939_tx_helper(
            tp->node_idx,
            J1939_TP_CM_PGN,
            (uint8_t*)&bam,
            J1939_TP_CM_LEN,
            tp->msg_info.dst,
            J1939_TP_CM_PRI);
    }
    else
    {
        tp->connection = J1939_TP_CONNECTION_P2P;

        struct J1939_TP_CM_RTS rts;
        j1939_tp_rts_pack(tp, &rts);
        j1939_tx_helper(
            tp->node_idx,
            J1939_TP_CM_PGN,
            (uint8_t*)&rts,
            J1939_TP_CM_LEN,
            tp->msg_info.dst,
            J1939_TP_CM_PRI);
    }

    return true;
}

void
j1939_tp_update(
    struct J1939TP* tp)
{
    if (!is_connection_active(tp))
        return;

    if (tp->connection == J1939_TP_CONNECTION_BROADCAST)
    {
        if (tp->sender)
            j1939_tp_broadcast_update_sender(tp);
        else
            j1939_tp_broadcast_update_receiver(tp);
    }
    else
    {
        if (tp->sender)
            j1939_tp_p2p_update_sender(tp);
        else
            j1939_tp_p2p_update_receiver(tp);
    }

    tp->timer_ms += tp->tick_rate_ms;
}

void
j1939_tp_close_connection(
    struct J1939TP* tp)
{
    tp->connection = J1939_TP_CONNECTION_NONE;
}

void
j1939_tp_dispatch(
    struct J1939TP* tp,
    struct J1939Msg* msg)
{
    if (msg->pgn == J1939_TP_DT_PGN)
    {
        if (!is_connection_active(tp))
            return;

        if (j1939_tp_rx_dt(tp, (struct J1939_TP_DT*)msg->data))
            tp->timer_ms = 0;
    }
    else
    {
        uint8_t control_byte = msg->data[0];

        // If there's an active connection and another node tries to open a
        //  connection, tell them to stop.
        if (is_connection_active(tp) &&
            ((control_byte == J1939_TP_CM_CONTROL_BYTE_RTS) ||
                (control_byte == J1939_TP_CM_CONTROL_BYTE_BAM)))
        {
            struct J1939_TP_CM_ABORT abort;

            // This cast assumes the PGN is in the 3 MSBs of the message
            j1939_tp_abort_pack(
                tp,
                &abort,
                J1939_TP_ABORT_REASON_BUSY,
                (uint32_t)msg->data[5]);
            j1939_tx_helper(
                tp->node_idx,
                J1939_TP_CM_PGN,
                (uint8_t*)&abort,
                J1939_TP_CM_LEN,
                msg->src,
                J1939_TP_CM_PRI);
            return;
        }

        switch (control_byte)
        {
        case J1939_TP_CM_CONTROL_BYTE_RTS:
            j1939_tp_rx_rts(tp, (struct J1939_TP_CM_RTS*)msg->data, msg->src);
            break;
        case J1939_TP_CM_CONTROL_BYTE_CTS:
            j1939_tp_rx_cts(tp, (struct J1939_TP_CM_CTS*)msg->data);
            break;
        case J1939_TP_CM_CONTROL_BYTE_ACK:
            j1939_tp_rx_ack(tp, (struct J1939_TP_CM_ACK*)msg->data);
            break;
        case J1939_TP_CM_CONTROL_BYTE_BAM:
            j1939_tp_rx_bam(tp, (struct J1939_TP_CM_BAM*)msg->data, msg->src);
            break;
        case J1939_TP_CM_CONTROL_BYTE_ABORT:
            j1939_tp_rx_abort(tp, (struct J1939_TP_CM_ABORT*)msg->data);
            break;
        }
    }
}

/* ============================================================================
 *
 * Section: Static function definitions
 *
 * ============================================================================
 */

static bool
is_connection_active(
    struct J1939TP* tp)
{
    return (tp->connection != J1939_TP_CONNECTION_NONE);
}
