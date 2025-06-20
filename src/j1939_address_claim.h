#pragma once

#include <stdint.h>
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

struct J1939AC {
    // Used for indexing into the global J1939Private array
    int node_idx;

    // Keeps a record of which addresses are available.
    // Each index into the table represents a valid node address (0 - 253).
    // If an address is available, the table value at the index will be zero.
    int address_table[J1939_AC_MAX_ADDRESSES];

    // Keeps track of the number of available slots in the address table
    int addresses_available;
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
    int node_idx);

bool
j1939_ac_update_address(
    struct J1939AC* ac);

