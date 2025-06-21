#pragma once

/* ============================================================================
 * File: j1939_address_claim.h
 *
 * Description: The address claim procedure allows for dynamic source address
 *              assignment. The general procedure is: on startup, each node
 *              sends out an ADDRESS_CLAIMED message, containing that node's
 *              NAME. If two or more nodes try claiming the same address, the
 *              node with a higher priority NAME get to keep theirs and the
 *              other node must select a new address. If all nodes start out
 *              with a unique address, there won't be any contention and thus
 *              no nodes will need to change their default address. For the
 *              procedure to work, each NODE in the network needs to have a
 *              unique NAME.
 * ============================================================================
 */

#include "j1939.h"

#include <stdbool.h>

/* ============================================================================
 *
 * Section: Macros
 *
 * ============================================================================
 */

// Number of valid node addresses (0 - 253)
#define J1939_AC_MAX_ADDRESSES  (254)

/* ============================================================================
 *
 * Section: Type definitions
 *
 * ============================================================================
 */

typedef struct J1939Name J1939_ADDRESS_CLAIMED;
#define J1939_ADDRESS_CLAIMED_PGN  (0x00EE00)
#define J1939_ADDRESS_CLAIMED_LEN  (8)
#define J1939_ADDRESS_CLAIMED_PRI  (6)

typedef J1939_ADDRESS_CLAIMED J1939_CANNOT_CLAIM_ADDRESS;
#define J1939_CANNOT_CLAIM_ADDRESS_PGN  (J1939_ADDRESS_CLAIMED_PGN)
#define J1939_CANNOT_CLAIM_ADDRESS_LEN  (J1939_ADDRESS_CLAIMED_LEN)
#define J1939_CANNOT_CLAIM_ADDRESS_PRI  (J1939_ADDRESS_CLAIMED_PRI)

// TODO: This is used to instruct a node with a given NAME to assume an address
struct __attribute__((packed)) J1939_COMMANDED_ADDRESS {
    struct J1939Name name;
    uint8_t new_source_address;
};
#define J1939_COMMANDED_ADDRESS_PGN  (0x00FED8)
#define J1939_COMMANDED_ADDRESS_LEN  (9)
#define J1939_COMMANDED_ADDRESS_PRI  (6)

struct J1939AC {
    // Used for indexing into the global J1939Private array
    int node_idx;

    // The NAME uniquely identifies a node and is used in arbitrating address
    //  contentions.
    struct J1939Name name;

    // Keeps a record of which addresses are available.
    // Each index into the table represents a valid node address (0 - 253).
    // If an address is available, the table value at the index will be zero.
    int address_table[J1939_AC_MAX_ADDRESSES];

    // Keeps track of the number of available slots in the address table
    int addresses_available;

    bool cannot_claim_address;
};

/* ============================================================================
 *
 * Section: Function prototypes
 *
 * ============================================================================
 */

void
j1939_ac_init(
    struct J1939AC* ac,
    int node_idx,
    struct J1939Name* name,
    J1939_AC_STARTUP_DELAY_250MS startup_delay,
    void* startup_delay_param);

// Return true if a new address is claimed; false otherwise.
bool
j1939_ac_update_address(
    struct J1939AC* ac);

void
j1939_ac_rx_address_claim(
    struct J1939AC* ac,
    struct J1939Msg* msg);

void
j1939_ac_rx_address_claim_request(
    struct J1939AC* ac);
