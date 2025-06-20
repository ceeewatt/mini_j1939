#pragma once

#include "j1939_define.h"

#include <stdint.h>
#include <stdbool.h>

struct J1939CanFrame {
    uint32_t id;
    uint8_t data[8];
    uint8_t len;
};

struct J1939Msg {
    uint32_t pgn;
    uint8_t* data;
    // With transport protocol, payload can be up to 1785 bytes
    uint16_t len;
    uint8_t src;
    uint8_t dst;
    uint8_t pri;
};

typedef bool (*J1939_CAN_RX)(struct J1939CanFrame*);
typedef bool (*J1939_CAN_TX)(struct J1939Msg*);
typedef void (*J1939_MSG_RX)(struct J1939Msg*);

struct J1939 {
    // An index given to this node; will be zero if there's just a single node
    int node_idx;
    uint8_t source_address;

    // The rate in ms at which the update function is called
    int tick_rate_ms;

    J1939_CAN_RX can_rx;
    J1939_CAN_TX can_tx;
    J1939_MSG_RX j1939_rx;
};

bool
j1939_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t preferred_address,
    int tick_rate_ms,
    J1939_CAN_RX can_rx,
    J1939_CAN_TX can_tx,
    J1939_MSG_RX j1939_rx);

void
j1939_update(
    struct J1939* node);

bool
j1939_tx(
    struct J1939* node,
    struct J1939Msg* msg);
