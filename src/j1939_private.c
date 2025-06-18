#include "j1939_private.h"

#ifndef J1939_NODES
#error "Set J1939_NODES to the number of Controller Applications used"
#endif

struct J1939Private g_j1939[J1939_NODES];

static void
dispatch(
    struct J1939* node,
    struct J1939Msg* msg)
{
    struct J1939Private* jp = &g_j1939[node->node_idx];

    switch (msg->pgn)
    {
        case J1939_TP_CM_PGN:
        case J1939_TP_DT_PGN:
            j1939_tp_dispatch(&jp->tp, msg);
            break;
        default:
            node->j1939_rx(msg);
            break;
    }
}

bool
j1939_init(
    struct J1939* node,
    int tick_rate_ms,
    J1939_CAN_RX can_rx,
    J1939_CAN_TX can_tx,
    J1939_MSG_RX j1939_rx)
{
    static int next_idx = 0;

    if (next_idx >= J1939_NODES)
        return false;

    node->node_idx = next_idx;
    g_j1939[next_idx].j1939_public = node;

    node->tick_rate_ms = tick_rate_ms;
    node->can_rx = can_rx;
    node->can_tx = can_tx;
    node->j1939_rx = j1939_rx;

    j1939_tp_init(&g_j1939[next_idx].tp, next_idx, tick_rate_ms, j1939_rx);

    next_idx++;
    return true;
}

void
j1939_update(
    struct J1939* node)
{
    struct J1939CanFrame frame;
    struct J1939Msg msg;
    uint8_t msg_buf[8];
    msg.data = msg_buf;

    while (node->can_rx(&frame))
    {
        // Sanity check
        if (frame.len > 8)
            continue;

        // TODO: pass frames along to application if application sets a config parameter?
        if (!j1939_can_frame_unpack(node, &frame, &msg))
            continue;

        // Ignore peer-to-peer messages not addressed to us
        if ((msg.dst != J1939_ADDR_GLOBAL) && (msg.dst != node->source_address))
            continue;

        dispatch(node, &msg);
    }
}

bool
j1939_tx(
    struct J1939* node,
    struct J1939Msg* msg)
{
    if (msg->len > 8)
    {
        struct J1939Private* jp = &g_j1939[node->node_idx];
        return j1939_tp_queue(&jp->tp, msg);
    }
    else
    {
        return node->can_tx(msg);
    }
}

bool
j1939_can_id_converter(
    struct CanIdConverter* converter,
    uint32_t id)
{
    converter->pri = (id >> 26) & 0x07;
    converter->dp = (id >> 24) & 0x01;
    converter->pf = (id >> 16) & 0xFF;
    converter->ps = (id >> 8) & 0xFF;
    converter->sa = id & 0xFF;

    if (converter->pf < 240)
    {
        // (PDU1) Destination-specific message: ps field is the destination address
        converter->pgn = (converter->dp << 16) | (converter->pf << 8);
        return false;
    }
    else
    {
        // (PDU2) Broadcast message: ps field is the LSB of the PGN
        converter->pgn = (converter->dp << 16) | (converter->pf << 8) | converter->ps;
        return true;
    }
}

// Return true if the CAN frame was successfully copied into the j1939 msg
bool
j1939_can_frame_unpack(
    struct J1939* node,
    struct J1939CanFrame* frame,
    struct J1939Msg* msg)
{
    // Caller should ensure extended frames have bit 31 set
    if (!((frame->id >> 31) & 1))
        return false;

    struct J1939Private* jp = &g_j1939[node->node_idx];

    if (j1939_can_id_converter(&jp->can_id_converter, frame->id))
        msg->dst = J1939_ADDR_GLOBAL;
    else
        msg->dst = jp->can_id_converter.ps;

    msg->pgn = jp->can_id_converter.pgn;
    msg->src = jp->can_id_converter.sa;
    msg->pri = jp->can_id_converter.pri;
    msg->len = frame->len;

    for (int i = 0; i < frame->len; ++i)
        msg->data[i] = frame->data[i];

    return true;
}
