#pragma once

/* ============================================================================
 * File: j1939.h
 *
 * Description: "Public" J1939 interface. The application layer should need
 *              only to include this header, and create/initialize a separate
 *              J1939 object for each unique node. The number of unique nodes
 *              operated by the application (ECU) should be defined by the
 *              J1939_NODES macro.
 *              If this library is compiled with the J1939_LISTENER_ONLY_MODE
 *              option enabled all nodes simply passively listen to the bus,
 *              meaning that:
 *              - Every extended CAN frame will be passed to application layer.
 *              - Nodes will not participate in address claim.
 *              - Message transmission is disallowed.
 *              However, CAN 2.0A messages will still be discarded.
 * ============================================================================
 */

#include "j1939_define.h"

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 *
 * Section: Type definitions
 *
 * ============================================================================
 */

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

/* ============================================================================
 * Subsection: Callback functions; implemented by application
 * ============================================================================
 */

// Receive a CAN frame from the bus into J1939CanFrame.
// Return false if there are no CAN frames to receive. Return true otherwise.
// This function is called at the rate specified by tick_rate_ms.
// The application must ensure that bit 31 of the CAN ID is set if the frame is
//  extended.
typedef bool (*J1939_CAN_RX)(struct J1939CanFrame*);

// Transmit a J1939Msg on the bus. Return true if successful, false otherwise.
typedef bool (*J1939_CAN_TX)(struct J1939Msg*);

// Receive a complete J1939Msg. This function is used for passing messages from
//  the J1939 layer to the application.
typedef void (*J1939_MSG_RX)(struct J1939Msg*);

// This function should implement a 250ms blocking delay. It accepts a single
//  parameter of any type.
typedef void (*J1939_AC_STARTUP_DELAY_250MS)(void*);

struct J1939 {
    // An index given to this node; will be zero if there's just a single node
    int node_idx;

    uint8_t source_address;

    // The rate in ms at which the update function is called
    int tick_rate_ms;

    J1939_CAN_RX can_rx;
    J1939_CAN_TX can_tx;
    J1939_MSG_RX j1939_rx;

    J1939_AC_STARTUP_DELAY_250MS startup_delay;
    void* startup_delay_param;

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

// Initialize a device node upon startup. This function should be called after
//  the CAN bus is up and operational. If the node is initialized without
//  issue, return true. Otherwise, return false. It is undefined behvavior to
//  attempt using a device node if that node was not initialized successfully.
// The J1939Name parameter should be unique and not used by any other node on
//  the network.
// Note that, depending on the preferred_address, this function may enter a
//  250ms blocking delay (via startup_delay parameter). The startup_delay_param
//  is an optional parameter that will be passed to the startup_delay callback.
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
    void* startup_delay_param);

// Call this function periodically, at the rate specified by the tick_rate_ms
//  init parameter.
void
j1939_update(
    struct J1939* node);

// Transmit a message on the bus, using the can_tx init parameter callback
//  function. Return true if successful, false otherwise.
bool
j1939_tx(
    struct J1939* node,
    struct J1939Msg* msg);

// Parses an extended (CAN2.0B) id and places the results in CanIdConverter.
// Returns true if id represents a broadcast message.
// Return false if id represents a destination-specific message, in which case
//  the destination of the corresponding j1939 message is the ps field.
bool
j1939_can_id_converter(
    struct CanIdConverter* converter,
    uint32_t id);

// Optional helper function for physical layer to derive CAN ID from J1939Msg
uint32_t
j1939_msg_to_can_id(
    const struct J1939Msg* msg);
