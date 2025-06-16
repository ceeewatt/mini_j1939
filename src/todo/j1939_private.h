#include "j1939.h"

/* j1939.c */

void
dispatch(
    struct J1939* node,
    struct J1939Msg* msg);

/* j1939_transport_protocol.c */

void
j1939_tp_init(
    struct J1939* node);

void
j1939_tp_close_connection(
    struct J1939* node);

// Attempt to queue a multi-byte message for transmission
bool
j1939_tp_queue(
    struct J1939* node,
    struct J1939Msg* msg);

void
j1939_tp_update(
    struct J1939* node);

// Receive a Connection Management packet
void
j1939_tp_rx_cm(
    struct J1939* node,
    struct J1939Msg* msg);

// Recieve a Data Transfer packet
void
j1939_tp_rx_dt(
    struct J1939* node,
    struct J1939Msg* msg);

/* j1939_address_claim.c */

void
j1939_ac_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t preferred_address,
    J1939_STARTUP_DELAY_250MS startup_delay,
    void* startup_delay_param);
void
j1939_ac_rx_address_claim(
    struct J1939* node,
    struct J1939Msg* msg);
void
j1939_ac_rx_address_claim_request(
    struct J1939* node,
    struct J1939Msg* msg);
