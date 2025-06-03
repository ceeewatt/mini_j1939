#pragma once

/* ============================================================================
 * File: j1939_transport_protocol_helper.h
 *
 * Description: A set of helper functions for the transport protocol layer.
 *              The functions are grouped into sections by whether they are
 *              used by a sender node, a receiver node, or both. Function
 *              names of the form j1939_tp_rx_* are used parsing received
 *              TP messages. Function names of the form j1939_tp_*_pack are
 *              used for preparing a given TP message to be transmitted.
 *              Here, you'll also find the periodic update functions,
 *              responsible for TP state machine logic for both sender/receiver
 *              broadcast and peer-to-peer connections.
 * ============================================================================
 */

#include "j1939_transport_protocol.h"

/* ============================================================================
 *
 * Section: Function prototypes
 *
 * ============================================================================
 */

/* ============================================================================
 * Subsection: Sender/receiver helper functions
 * ============================================================================
 */

// Create a J1939 msg and transmit it, used for sending TP.DT and TP.CM msgs
void
j1939_tp_tx(
    struct J1939TP* tp,
    uint32_t pgn,
    uint8_t* data,
    uint16_t len,
    uint8_t dst,
    uint8_t pri);

void
j1939_tp_rx_abort(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort);

void
j1939_tp_abort_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort,
    enum j1939_tp_abort_reason reason,
    uint32_t pgn);

/* ============================================================================
 * Subsection: Sender helper functions
 * ============================================================================
 */

void
j1939_tp_broadcast_update_sender(
    struct J1939TP* tp);

void
j1939_tp_p2p_update_sender(
    struct J1939TP* tp);

void
j1939_tp_dt_pack(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);

void j1939_tp_rts_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_RTS* rts);

void
j1939_tp_rx_cts(
    struct J1939TP* tp,
    struct J1939_TP_CM_CTS* cts);

void
j1939_tp_rx_ack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ACK* ack);

void
j1939_tp_bam_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam);

/* ============================================================================
 * Subsection: Receiver helper functions
 * ============================================================================
 */

void
j1939_tp_broadcast_update_receiver(
    struct J1939TP* tp);

void
j1939_tp_p2p_update_receiver(
    struct J1939TP* tp);

// Return false if TP.DT message is not received successfully
bool
j1939_tp_rx_dt(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);

void
j1939_tp_rx_rts(
    struct J1939TP* tp,
    struct J1939_TP_CM_RTS* rts,
    uint8_t msg_src);

void
j1939_tp_cts_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_CTS* cts);

void
j1939_tp_ack_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ACK* ack);

void
j1939_tp_rx_bam(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam,
    uint8_t msg_src);
