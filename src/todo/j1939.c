#include "j1939.h"
#include "j1939_private.h"
#include <stddef.h>

#ifdef J1939_UNIT_TEST
    #define J1939_STATIC 
#else
    #define J1939_STATIC static
#endif

J1939_STATIC void
rx_request(
    struct J1939* node,
    struct J1939Msg* msg)
{
    uint32_t requested_pgn = *((uint32_t*)msg->data);

    switch (requested_pgn)
    {
        case J1939_ADDRESS_CLAIMED_PGN:
#ifndef J1939_DISABLE_ADDRESS_CLAIM
            j1939_ac_rx_address_claim_request(node, msg);
#endif
            break;
        default:
            node->priv.app_rx(msg);
            break;
    }
}

// Get the msg to its proper handler
void
dispatch(
    struct J1939* node,
    struct J1939Msg* msg)
{
    switch (msg->pgn)
    {
#ifndef J1939_DISABLE_ADDRESS_CLAIM
        case J1939_ADDRESS_CLAIMED_PGN:
            j1939_ac_rx_address_claim(node, msg);
            break;
#endif
#ifndef J1939_DISABLE_TRANSPORT_PROTOCOL
        case J1939_TP_CM_PGN:
            j1939_tp_rx_cm(node, msg);
            break;
        case J1939_TP_DT_PGN:
            j1939_tp_rx_dt(node, msg);
            break;
#endif
        case J1939_REQUEST_PGN:
            rx_request(node, msg);
            break;
        default:
            // Pass to application
            node->priv.app_rx(msg);
            break;
    }
}

// First level of inspection for the CAN frame
J1939_STATIC void
inspect_can_frame(
    struct J1939* node,
    struct J1939Msg* msg,
    struct J1939CanFrame* frame)
{
    // Don't handle CAN 2.0A frames
    if (!((frame->can_id >> 31) & 1)){
        return;
    }

    uint8_t priority = (frame->can_id >> 26) & 0x07;
    uint8_t dp = (frame->can_id >> 24) & 1;
    uint8_t pf = (frame->can_id >> 16) & 0xFF;
    uint8_t ps = (frame->can_id >> 8) & 0xFF;
    uint8_t sa = frame->can_id & 0xFF;
    uint32_t pgn;

    if (pf < 240)
    {
        // Only handle destination-specific msg if addressed to us
        if ((ps != J1939_ADDR_GLOBAL) && (ps != node->priv.source_address))
            return;

        pgn = (dp << 16) | (pf << 8);
        msg->dst = ps;
    }
    else
    {
        pgn = (dp << 16) | (pf << 8) | ps;
        msg->dst = J1939_ADDR_GLOBAL;
    }

    msg->pgn = pgn;
    msg->len = frame->len;
    msg->src = sa;
    msg->pri = priority;

    for (int i = 0; i < msg->len; ++i)
        msg->data[i] = frame->data[i];

    dispatch(node, msg);
}

bool
j1939_init(
    struct J1939* node,
    struct J1939Name* name,
    J1939_PHYSICAL_RX physical_rx,
    J1939_PHYSICAL_TX physical_tx,
    J1939_APP_RX app_rx,
    J1939_STARTUP_DELAY_250MS startup_delay,
    void* startup_delay_param,
    uint8_t preferred_address,
    int tick_rate_ms)
{
    if (preferred_address > 253)
        return false;

    if (!physical_rx || !physical_tx|| !app_rx)
        return false;

    node->priv.physical_rx = physical_rx;
    node->priv.physical_tx = physical_tx;
    node->priv.app_rx = app_rx;
    node->priv.tick_rate_ms = tick_rate_ms;

#ifndef J1939_DISABLE_ADDRESS_CLAIM
    if (!startup_delay)
        return false;

    j1939_ac_init(node, name, preferred_address, startup_delay, startup_delay_param);
#else
    (void)name;
    (void)startup_delay;
    (void)startup_delay_param;
    node->priv.source_address = preferred_address;
#endif

#ifndef J1939_DISABLE_TRANSPORT_PROTOCOL
    j1939_tp_init(node);
#endif

    return true;
}

void
j1939_update(
    struct J1939* node)
{
    struct J1939CanFrame frame;
    uint8_t buf[8];
    struct J1939Msg msg;
    msg.data = buf;

    while (node->priv.physical_rx(&frame)) {
        inspect_can_frame(node, &msg, &frame);
    }

#ifndef J1939_DISABLE_TRANSPORT_PROTOCOL
    j1939_tp_update(node);
#endif
}

void
j1939_tx(
    struct J1939* node,
    struct J1939Msg* msg)
{
#ifndef J1939_DISABLE_ADDRESS_CLAIM
    if (node->ac.cannot_claim_address)
        return;
#endif

    msg->src = node->priv.source_address;

    if (msg->len <= 8)
        node->priv.physical_tx(msg); 
#ifndef J1939_DISABLE_TRANSPORT_PROTOCOL
    else
        j1939_tp_queue(node, msg);
#endif
}
