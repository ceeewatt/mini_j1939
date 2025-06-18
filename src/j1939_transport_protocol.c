#include "j1939_transport_protocol.h"
#include "j1939_private.h"

static bool
is_connection_active(
    struct J1939TP* tp)
{
    return (tp->connection != J1939_TP_CONNECTION_NONE);
}

static void
tp_tx(
    struct J1939TP* tp,
    uint32_t pgn,
    uint8_t* data,
    uint16_t len,
    uint8_t dst,
    uint8_t pri)
{
    struct J1939Private* jp = &g_j1939[tp->node_idx];
    struct J1939Msg msg;

    msg.pgn = pgn;
    msg.data = data;
    msg.len = len;
    msg.dst = dst;
    msg.pri = pri;

    (void)j1939_tx(jp->j1939_public, &msg);
}

static void
timeout(
    struct J1939TP* tp)
{
    struct J1939_TP_CM_ABORT abort;

    j1939_tp_abort_pack(tp, &abort, J1939_TP_ABORT_REASON_TIMEOUT, tp->msg_info.pgn);
    j1939_tp_close_connection(tp);
}

static void
broadcast_update(
    struct J1939TP* tp)
{
    if (tp->sender)
    {
        if (tp->bytes_rem)
        {
            if (tp->timer_ms >= J1939_TP_TX_PERIOD)
            {
                struct J1939_TP_DT dt;
                j1939_tp_dt_pack(tp, &dt);
                tp_tx(
                    tp,
                    J1939_TP_DT_PGN,
                    (uint8_t*)&dt,
                    J1939_TP_DT_LEN,
                    J1939_ADDR_GLOBAL,
                    J1939_TP_DT_PRI);
            }
        }
        else
        {
            j1939_tp_close_connection(tp);
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
            tp->j1939_rx(&tp->msg_info);
            j1939_tp_close_connection(tp);
        }
    }
}

static void
p2p_update(
    struct J1939TP* tp)
{
    if (tp->sender)
    {
        // TODO: set clear_to_send = true when we receive a CTS
        if (tp->clear_to_send)
        {
            if (tp->bytes_rem)
            {
                if (tp->timer_ms >= J1939_TP_TX_PERIOD)
                {
                    struct J1939_TP_DT dt;
                    j1939_tp_dt_pack(tp, &dt);
                    tp_tx(
                        tp,
                        J1939_TP_DT_PGN,
                        (uint8_t*)&dt,
                        J1939_TP_DT_LEN,
                        // TODO: send this to the address of the receiver node
                        //  whenever a p2p connection is opened (by j1939_tx()), we need to save the msg.dst into tp.receiver_address
                        tp->receiver_address,
                        J1939_TP_DT_PRI);
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
    else
    {
        if (!tp->clear_to_send)
        {
            if (tp->timer_ms >= J1939_TP_TX_PERIOD)
            {
                // TODO
                // send CTS
                // tp->clear_to_send = true;
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
                tp_tx(
                    tp,
                    J1939_TP_CM_PGN,
                    (uint8_t*)&ack,
                    J1939_TP_CM_LEN,
                    tp->msg_info.src,
                    J1939_TP_CM_PRI);
                j1939_tp_close_connection(tp);
            }
        }
    }
}

void
j1939_tp_update(
    struct J1939TP* tp)
{
    if (!is_connection_active(tp))
        return;

    if (tp->connection == J1939_TP_CONNECTION_BROADCAST)
        broadcast_update(tp);
    else
        p2p_update(tp);

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

            tp_tx(
                tp,
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
            // TODO: make sure we save the source address of the node sending the RTS
                // msg_info.src
            break;
        case J1939_TP_CM_CONTROL_BYTE_CTS:
            break;
        case J1939_TP_CM_CONTROL_BYTE_ACK:
            break;
        case J1939_TP_CM_CONTROL_BYTE_BAM:
            j1939_tp_rx_bam(tp, (struct J1939_TP_CM_BAM*)msg->data, msg->src);
            break;
        case J1939_TP_CM_CONTROL_BYTE_ABORT:
            break;
        }
    }
}

void
j1939_tp_init(
    struct J1939TP* tp,
    int node_idx,
    int tick_rate_ms,
    J1939_MSG_RX j1939_rx)
{
    tp->node_idx = node_idx;

    tp->connection = J1939_TP_CONNECTION_NONE;
    tp->msg_info.data = tp->buf;
    tp->tick_rate_ms = tick_rate_ms;
    tp->j1939_rx = j1939_rx;
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

void
j1939_tp_rx_rts(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam,
    uint8_t msg_src)
{
    // TODO: after we receive an RTS, we need to reply with a CTS message
    // send CTS immediately?


    struct J1939Private* jp = &g_j1939[tp->node_idx];

    tp->connection = J1939_TP_CONNECTION_P2P;
    tp->sender = false;

    tp->next_seq = 1;
    tp->bytes_rem = bam->len;
    tp->num_packages = bam->num_packages;

    tp->msg_info.pgn = bam->pgn;
    tp->msg_info.len = bam->len;
    tp->msg_info.src = msg_src;
    tp->msg_info.dst = jp->j1939_public->source_address;

    // TODO: pgn lookup to determine priority
    tp->msg_info.pri = J1939_DEFAULT_PRIORITY;

    // CTS message will be sent in update loop
    tp->clear_to_send = false;
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

void
j1939_tp_rx_abort(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort)
{
    if (abort->pgn == tp->msg_info.pgn)
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
