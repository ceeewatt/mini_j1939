#include "j1939_transport_protocol_helper.h"
#include "j1939_private.h"

/* ============================================================================
 *
 * Section: Static function prototypes
 *
 * ============================================================================
 */

static void
timeout(
    struct J1939TP* tp);

/* ============================================================================
 *
 * Section: Function definitions
 *
 * ============================================================================
 */

/* ============================================================================
 * Subsection: Sender/receiver helper functions
 * ============================================================================
 */

void
j1939_tp_rx_abort(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort)
{
    if (abort->pgn == tp->msg_info.pgn)
        j1939_tp_close_connection(tp);
}

void
j1939_tp_abort_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort,
    enum j1939_tp_abort_reason reason,
    uint32_t pgn)
{
    (void)tp;

    abort->control_byte = J1939_TP_CM_CONTROL_BYTE_ABORT;
    abort->abort_reason = reason;
    abort->res = 0xFF;
    abort->pgn = pgn;
}

/* ============================================================================
 * Subsection: Sender helper functions
 * ============================================================================
 */

void
j1939_tp_broadcast_update_sender(
    struct J1939TP* tp)
{
    if (tp->bytes_rem)
    {
        if (tp->timer_ms >= J1939_TP_TX_PERIOD)
        {
            struct J1939_TP_DT dt;
            j1939_tp_dt_pack(tp, &dt);
            j1939_tx_helper(
                tp->node_idx,
                J1939_TP_DT_PGN,
                (uint8_t*)&dt,
                J1939_TP_DT_LEN,
                tp->msg_info.dst,
                J1939_TP_DT_PRI);
            tp->timer_ms = 0;
        }
    }
    else
    {
        j1939_tp_close_connection(tp);
    }
}

void
j1939_tp_p2p_update_sender(
    struct J1939TP* tp)
{
    if (tp->clear_to_send)
    {
        if (tp->bytes_rem)
        {
            if (tp->timer_ms >= J1939_TP_TX_PERIOD)
            {
                struct J1939_TP_DT dt;
                j1939_tp_dt_pack(tp, &dt);
                j1939_tx_helper(
                    tp->node_idx,
                    J1939_TP_DT_PGN,
                    (uint8_t*)&dt,
                    J1939_TP_DT_LEN,
                    tp->msg_info.dst,
                    J1939_TP_DT_PRI);
                tp->timer_ms = 0;
            }
        }
        else
        {
            if (tp->timer_ms >= J1939_TP_TIMEOUT_T3)
                timeout(tp);
        }
    }
    else
    {
        if (tp->timer_ms >= J1939_TP_TIMEOUT_TR)
            timeout(tp);
    }
}

void
j1939_tp_dt_pack(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt)
{
    uint8_t* buf = tp->buf + ((tp->next_seq - 1) * 7);
    uint8_t* data = (uint8_t*)&dt->data0;
    int bytes_to_copy = (tp->bytes_rem < 7) ? tp->bytes_rem : 7;

    int i;
    for (i = 0; i < bytes_to_copy; ++i)
    {
        data[i] = buf[i];
        tp->bytes_rem--;
    }

    // We've no more bytes remaining, fill unused data bytes with 0xFF
    for (; i < 7; ++i)
        data[i] = 0xFF;

    dt->seq = tp->next_seq;
    tp->next_seq++;
}

void j1939_tp_rts_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_RTS* rts)
{
    rts->control_byte = J1939_TP_CM_CONTROL_BYTE_RTS;
    rts->len = tp->msg_info.len;
    rts->num_packages = tp->num_packages;
    rts->max_packages = J1939_TP_CM_RTS_MAX_PACKAGES;
    rts->pgn = tp->msg_info.pgn;
}

void j1939_tp_rx_cts(
    struct J1939TP* tp,
    struct J1939_TP_CM_CTS* cts)
{
    // NOTE: the standard defines a method by which the receiver can delay the
    //  data transmission while keeping the connection open. This is done by
    //  sending a CTS message, with num_packages set to zero, every 0.5 seconds
    //  until it's ready to receive the data. I'm choosing not to implement this.
    //  Thus, when the sender receives a CTS message, it's assumed that the
    //  receiver is ready to receive all bytes of payload.
    (void)cts;

    tp->clear_to_send = true;
}

void
j1939_tp_rx_ack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ACK* ack)
{
    if ((ack->pgn == tp->msg_info.pgn) && (tp->bytes_rem == 0))
        j1939_tp_close_connection(tp);
}

void
j1939_tp_bam_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam)
{
    bam->control_byte = J1939_TP_CM_CONTROL_BYTE_BAM;
    bam->len = tp->msg_info.len;
    bam->num_packages = tp->num_packages;
    bam->res = 0xFF;
    bam->pgn = tp->msg_info.pgn;
}

/* ============================================================================
 * Subsection: Receiver helper functions
 * ============================================================================
 */

void
j1939_tp_broadcast_update_receiver(
    struct J1939TP* tp)
{
    if (tp->timer_ms >= J1939_TP_TIMEOUT_T1)
    {
        timeout(tp);
    }
    else if (tp->bytes_rem == 0)
    {
        j1939_rx_helper(tp->node_idx, &tp->msg_info);
        j1939_tp_close_connection(tp);
    }
}

void
j1939_tp_p2p_update_receiver(
    struct J1939TP* tp)
{
    if (!tp->clear_to_send)
    {
        if (tp->timer_ms >= J1939_TP_TX_PERIOD)
        {
            struct J1939_TP_CM_CTS cts;
            j1939_tp_cts_pack(tp, &cts);
            j1939_tx_helper(
                tp->node_idx,
                J1939_TP_CM_PGN,
                (uint8_t*)&cts,
                J1939_TP_CM_LEN,
                tp->msg_info.src,
                J1939_TP_CM_PRI);
            tp->timer_ms = 0;
            tp->clear_to_send = true;
        }
    }
    else
    {
        if (tp->timer_ms >= J1939_TP_TIMEOUT_T1)
        {
            timeout(tp);
        }
        else if (tp->bytes_rem == 0)
        {
            struct J1939_TP_CM_ACK ack;
            j1939_tp_ack_pack(tp, &ack);
            j1939_tx_helper(
                tp->node_idx,
                J1939_TP_CM_PGN,
                (uint8_t*)&ack,
                J1939_TP_CM_LEN,
                tp->msg_info.src,
                J1939_TP_CM_PRI);

            j1939_rx_helper(tp->node_idx, &tp->msg_info);
            j1939_tp_close_connection(tp);
        }
    }
}

bool
j1939_tp_rx_dt(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt)
{
    if (dt->seq != tp->next_seq)
        return false;

    uint8_t* buf = tp->buf + ((tp->next_seq - 1) * 7);
    uint8_t* data = (uint8_t*)&dt->data0;
    int bytes_to_copy = (tp->bytes_rem < 7) ? tp->bytes_rem : 7;

    for (int i = 0; i < bytes_to_copy; ++i)
    {
        buf[i] = data[i];
        tp->bytes_rem--;
    }

    tp->next_seq++;
    return true;
}

void
j1939_tp_rx_rts(
    struct J1939TP* tp,
    struct J1939_TP_CM_RTS* rts,
    uint8_t msg_src)
{
    tp->connection = J1939_TP_CONNECTION_P2P;
    tp->sender = false;

    tp->next_seq = 1;
    tp->bytes_rem = rts->len;
    tp->num_packages = rts->num_packages;

    tp->msg_info.pgn = rts->pgn;
    tp->msg_info.len = rts->len;
    tp->msg_info.src = msg_src;
    tp->msg_info.dst = j1939_get_source_address(tp->node_idx);

    // TODO: pgn lookup to determine priority
    tp->msg_info.pri = J1939_DEFAULT_PRIORITY;

    // CTS message will be sent in update loop
    tp->clear_to_send = false;
}

void j1939_tp_cts_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_CTS* cts)
{
    cts->control_byte = J1939_TP_CM_CONTROL_BYTE_CTS;
    cts->num_packages = tp->num_packages;
    cts->next_seq = 1;
    cts->res = 0xFFFF;
    cts->pgn = tp->msg_info.pgn;
}

void
j1939_tp_ack_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ACK* ack)
{
    ack->control_byte = J1939_TP_CM_CONTROL_BYTE_ACK;
    ack->len = tp->msg_info.len;
    ack->num_packages = tp->num_packages;
    ack->res = 0xFF;
    ack->pgn = tp->msg_info.pgn;
}

void
j1939_tp_rx_bam(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam,
    uint8_t msg_src)
{
    tp->connection = J1939_TP_CONNECTION_BROADCAST;
    tp->sender = false;

    tp->next_seq = 1;
    tp->bytes_rem = bam->len;
    tp->num_packages = bam->num_packages;

    tp->msg_info.pgn = bam->pgn;
    tp->msg_info.len = bam->len;
    tp->msg_info.src = msg_src;
    tp->msg_info.dst = J1939_ADDR_GLOBAL;

    // TODO: pgn lookup to determine priority
    tp->msg_info.pri = J1939_DEFAULT_PRIORITY;
}

/* ============================================================================
 *
 * Section: Static function definitions
 *
 * ============================================================================
 */

static void
timeout(
    struct J1939TP* tp)
{
    struct J1939_TP_CM_ABORT abort;

    j1939_tp_abort_pack(tp, &abort, J1939_TP_ABORT_REASON_TIMEOUT, tp->msg_info.pgn);
    j1939_tx_helper(
        tp->node_idx,
        J1939_TP_CM_PGN,
        (uint8_t*)&abort,
        J1939_TP_CM_LEN,
        tp->msg_info.dst,
        J1939_TP_CM_PRI);
    j1939_tp_close_connection(tp);
}
