#pragma once

#include "j1939_private.h"

/*
 * GENERIC HELPER FUNCTIONS
*/
void
tp_tx(
    struct J1939TP* tp,
    uint32_t pgn,
    uint8_t* data,
    uint16_t len,
    uint8_t dst,
    uint8_t pri);

/*
* SENDER HELPER FUNCTIONS
*
*/
void
broadcast_update_sender(
    struct J1939TP* tp);
void
p2p_update_sender(
    struct J1939TP* tp);

void
j1939_tp_dt_pack(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);
void j1939_tp_rts_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_RTS* rts);
void j1939_tp_cts_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_CTS* cts);
void
j1939_tp_ack_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ACK* ack);
void
j1939_tp_bam_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam);
void
j1939_tp_abort_pack(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort,
    enum j1939_tp_abort_reason reason,
    uint32_t pgn);

/*
* RECIEVER HELPER FUNCTIONS
*
*/
void
broadcast_update_receiver(
    struct J1939TP* tp);
void
p2p_update_receiver(
    struct J1939TP* tp);

bool
j1939_tp_rx_dt(
    struct J1939TP* tp,
    struct J1939_TP_DT* dt);
void
j1939_tp_rx_rts(
    struct J1939TP* tp,
    struct J1939_TP_CM_RTS* rts,
    uint8_t msg_src);
void j1939_tp_rx_cts(
    struct J1939TP* tp,
    struct J1939_TP_CM_CTS* cts);
// TODO ACK
void
j1939_tp_rx_bam(
    struct J1939TP* tp,
    struct J1939_TP_CM_BAM* bam,
    uint8_t msg_src);
void
j1939_tp_rx_abort(
    struct J1939TP* tp,
    struct J1939_TP_CM_ABORT* abort);
