#include "j1939_private.h"

#include <string.h>

#ifndef J1939_NODES
#error "Set J1939_NODES to the number of Controller Applications used"
#endif

#ifdef J1939_LISTENER_ONLY_MODE
#pragma message "Compiling with J1939_LISTENER_ONLY_MODE enabled... message transmission is disabled"
#endif

// Remove static qualifier to make variable accessible from unit tests
#ifdef UNIT_TEST
struct J1939Private g_j1939[J1939_NODES];
#else
static struct J1939Private g_j1939[J1939_NODES];
#endif

/* ============================================================================
 *
 * Section: Static function prototypes
 *
 * ============================================================================
 */

static void
dispatch(
    struct J1939* node,
    struct J1939Msg* msg);

/* ============================================================================
 *
 * Section: Function definitions
 *
 * ============================================================================
 */

/* ============================================================================
 * Subsection: Public function definitions
 * ============================================================================
 */

bool
j1939_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t preferred_address,
    int tick_rate_ms,
    J1939_CAN_RX can_rx,
    J1939_CAN_TX can_tx,
    J1939_MSG_RX j1939_rx,
    J1939_AC_STARTUP_DELAY_250MS startup_delay,
    void* startup_delay_param)
{
    static int next_idx = 0;

    if (next_idx >= J1939_NODES)
        return false;

    if ((node == NULL) || (name == NULL))
        return false;

    if (preferred_address >= J1939_AC_MAX_ADDRESSES)
        return false;

    if (tick_rate_ms <= 0)
        return false;

    if ((startup_delay == NULL)  ||
        (j1939_rx == NULL)       ||
        (can_rx == NULL)         ||
        (can_tx == NULL))
    {
        return false;
    }

    node->node_idx = next_idx;
    g_j1939[next_idx].j1939_public = node;

    node->source_address = preferred_address;
    node->tick_rate_ms = tick_rate_ms;
    node->can_rx = can_rx;
    node->can_tx = can_tx;
    node->j1939_rx = j1939_rx;

    j1939_tp_init(
        &g_j1939[next_idx].tp,
        next_idx,
        tick_rate_ms);

#ifndef J1939_LISTENER_ONLY_MODE
    j1939_ac_init(
        &g_j1939[next_idx].ac,
        next_idx,
        name,
        startup_delay,
        startup_delay_param);
#else
    (void)startup_delay_param;
#endif

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

        if (!j1939_can_frame_unpack(node, &frame, &msg))
            continue;

        // Ignore peer-to-peer messages not addressed to us
        if ((msg.dst != J1939_ADDR_GLOBAL) && (msg.dst != node->source_address))
        {
        #ifdef J1939_LISTENER_ONLY_MODE
            node->j1939_rx(&msg);
        #else
            continue;
        #endif
        }

        dispatch(node, &msg);
    }

    j1939_tp_update(&g_j1939[node->node_idx].tp);
}

bool
j1939_tx(
    struct J1939* node,
    struct J1939Msg* msg)
{
#ifndef J1939_LISTENER_ONLY_MODE
    if (g_j1939[node->node_idx].ac.cannot_claim_address)
        return false;

    msg->src = node->source_address;

    if (msg->len > 8)
    {
        struct J1939Private* jp = &g_j1939[node->node_idx];
        return j1939_tp_queue(&jp->tp, msg);
    }
    else
    {
        return node->can_tx(msg);
    }
#else
    (void)node, (void)msg;
    return false;
#endif
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

uint32_t
j1939_msg_to_can_id(
    struct J1939Msg* msg)
{
    uint8_t dp = (msg->pgn >> 16) & 0x01;
    uint8_t pf = (msg->pgn >> 8) & 0xFF;
    uint8_t ps = (pf < 240) ? msg->dst : (msg->pgn & 0xFF);

    uint32_t can_id =
        (1 << 31)         |
        (msg->pri << 26)  |
        (dp << 24)        |
        (pf << 16)        |
        (ps << 8)         |
        msg->src;

    return can_id;
}

/* ============================================================================
 * Subsection: Private function definitions
 * ============================================================================
 */

bool
j1939_can_frame_unpack(
    struct J1939* node,
    struct J1939CanFrame* frame,
    struct J1939Msg* msg)
{
    // Caller should ensure extended frames have bit 31 set
    if (!((frame->id >> 31) & 1))
        return false;

    if (j1939_can_id_converter(&node->can_id_converter, frame->id))
        msg->dst = J1939_ADDR_GLOBAL;
    else
        msg->dst = node->can_id_converter.ps;

    msg->pgn = node->can_id_converter.pgn;
    msg->src = node->can_id_converter.sa;
    msg->pri = node->can_id_converter.pri;
    msg->len = frame->len;

    memcpy(msg->data, frame->data, frame->len);

    return true;
}

/* ============================================================================
 * Subsection: helper functions for transport protocol and address claim
 * ============================================================================
 */

void
j1939_tx_helper(
    int node_idx,
    uint32_t pgn,
    uint8_t* data,
    uint16_t len,
    uint8_t dst,
    uint8_t pri)
{
    struct J1939Private* jp = &g_j1939[node_idx];
    struct J1939Msg msg = {
        .pgn = pgn,
        .data = data,
        .len = len,
        .dst = dst,
        .pri = pri
    };

    (void)j1939_tx(jp->j1939_public, &msg);
}

void
j1939_rx_helper(
    int node_idx,
    struct J1939Msg* msg)
{
    g_j1939[node_idx].j1939_public->j1939_rx(msg);
}

void
j1939_set_source_address(
    int node_idx,
    uint8_t new_address)
{
    g_j1939[node_idx].j1939_public->source_address = new_address;
}

uint8_t
j1939_get_source_address(
    int node_idx)
{
    return g_j1939[node_idx].j1939_public->source_address;
}

void
j1939_close_transport_protocol_connection(
    int node_idx)
{
    j1939_tp_close_connection(&g_j1939[node_idx].tp);
}

/* ============================================================================
 *
 * Section: Static function definitions
 *
 * ============================================================================
 */

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

#ifndef J1939_LISTENER_ONLY_MODE
    case J1939_ADDRESS_CLAIMED_PGN:
        j1939_ac_rx_address_claim(&jp->ac, msg);
        break;
    case J1939_REQUEST_PGN:
        if (((struct J1939_REQUEST*)msg->data)->pgn == J1939_ADDRESS_CLAIMED_PGN)
        {
            j1939_ac_rx_address_claim_request(&jp->ac);
            break;
        }
    // Fallthrough to application
    __attribute__((fallthrough));
#endif

    default:
        node->j1939_rx(msg);
        break;
    }
}
