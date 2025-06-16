#include "j1939_private.h"

#ifndef J1939_NODES
#error "Set J1939_NODES to the number of Controller Applications used."
#endif

static struct J1939Private g_j1939[J1939_NODES];

bool
j1939_init(
    struct J1939* node)
{
    static int next_idx = 0;

    if (next_idx >= J1939_NODES)
        return false;

    node->node_idx = next_idx;
    g_j1939[next_idx].j1939_public = node;

    next_idx++;
    return true;
}

bool
can_id_converter(
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

bool
can_frame_to_j1939_message(
    struct J1939* node,
    struct J1939CanFrame* frame,
    struct J1939Msg* msg)
{
    // Caller should ensure extended frames have bit 31 set
    if (!((frame->id >> 31) & 1))
        return false;

    // Sanity check
    if (frame->len > 8)
        return false;

    struct J1939Private* jp = &g_j1939[node->node_idx];

    if (can_id_converter(&jp->can_id_converter, frame->id))
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
