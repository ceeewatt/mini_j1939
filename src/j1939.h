#pragma once

#include <stdint.h>
#include <stdbool.h>

#define J1939_ADDR_GLOBAL (255)

struct J1939 {
    // An index given to this node; will be zero if there's just a single node
    int node_idx;
    uint8_t source_address;
};

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

bool
j1939_init(
    struct J1939* node);
