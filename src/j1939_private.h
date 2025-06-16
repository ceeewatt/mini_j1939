#pragma once

#include "j1939.h"

struct J1939Private {
    struct J1939* j1939_public;

    // TODO: private data...

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
can_id_converter(
    struct CanIdConverter* converter,
    uint32_t id);

bool
can_frame_to_j1939_message(
    struct J1939* node,
    struct J1939CanFrame* frame,
    struct J1939Msg* msg);


#ifdef UNIT_TEST
extern struct J1939Private* p_g_j1939;
#endif
