#pragma once

#include "j1939.h"
#include "j1939_transport_protocol.h"

struct J1939Private {
    struct J1939* j1939_public;

    struct J1939TP tp;

    // CAN ID fields of the most recently processed CAN frame
    struct CanIdConverter {
        uint8_t pri;
        uint8_t dp;
        uint8_t pf;
        uint8_t ps;
        uint8_t sa;
        uint32_t pgn;
    } can_id_converter;
};

// Returns true if the id represents a broadcast message
// Otherwise, the id represents a destination-specific frame
//  in which case, the destintation of the corresponding j1939 message is the
//  'ps' field.
bool
j1939_can_id_converter(
    struct CanIdConverter* converter,
    uint32_t id);

bool
j1939_can_frame_unpack(
    struct J1939* node,
    struct J1939CanFrame* frame,
    struct J1939Msg* msg);



extern struct J1939Private g_j1939[];

