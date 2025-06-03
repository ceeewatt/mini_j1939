#pragma once

/* ============================================================================
 * File: j1939_private.h
 *
 * Description: "Private" J1939 implementation. The J1939Private struct holds
 *              a pointer to a J1939 struct, created by the application layer
 *              and passed as a parameter to the top-level init function.
 *              The corresponding source file defines a static global array of
 *              J1939Private objects. Other translation units (transport
 *              protocol and address claim) are linked to a global J1939Private
 *              object via a node_idx variable, which indexes into the global
 *              array. This header file provides helper function to those
 *              translation units for interfacing with this "private" data.
 * ============================================================================
 */

#include "j1939.h"
#include "j1939_transport_protocol.h"
#include "j1939_address_claim.h"

/* ============================================================================
 *
 * Section: Type definitions
 *
 * ============================================================================
 */

struct J1939Private {
    struct J1939* j1939_public;

    struct J1939TP tp;

    struct J1939AC ac;

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

/* ============================================================================
 *
 * Section: Function prototypes
 *
 * ============================================================================
 */

// Returns true if the id represents a broadcast message
// Otherwise, the id represents a destination-specific frame
//  in which case, the destintation of the corresponding j1939 message is the
//  'ps' field.
bool
j1939_can_id_converter(
    struct CanIdConverter* converter,
    uint32_t id);

// Return false if the CAN frame is a standard frame (CAN 2.0A); true otherwise
bool
j1939_can_frame_unpack(
    struct J1939* node,
    struct J1939CanFrame* frame,
    struct J1939Msg* msg);

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
    uint8_t pri);

void
j1939_rx_helper(
    int node_idx,
    struct J1939Msg* msg);

void
j1939_set_source_address(
    int node_idx,
    uint8_t new_address);

uint8_t
j1939_get_source_address(
    int node_idx);

void
j1939_close_transport_protocol_connection(
    int node_idx);
