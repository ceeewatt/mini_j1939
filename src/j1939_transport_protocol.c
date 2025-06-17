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

        j1939_tp_rx_dt(tp, (struct J1939_TP_DT*)msg->data);
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
    int node_idx)
{
    tp->node_idx = node_idx;

    tp->connection = J1939_TP_CONNECTION_NONE;
    tp->msg_info.data = tp->buf;
}

void
j1939_tp_rx_dt(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt)
{
    if (dt->seq != tp->next_seq)
        return;

    uint8_t* buf = tp->buf + ((tp->next_seq - 1) * 7);
    uint8_t* data = (uint8_t*)&dt->data0;
    int bytes_to_copy = (tp->bytes_rem < 7) ? tp->bytes_rem : 7;

    for (int i = 0; i < bytes_to_copy; ++i)
    {
        buf[i] = data[i];
        tp->bytes_rem--;
    }

    tp->next_seq++;
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
    if (abort->pgn != tp->msg_info.pgn)
        return;
}
