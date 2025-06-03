#include "j1939_address_claim.h"
#include "j1939_private.h"

#include <string.h>

/* ============================================================================
 *
 * Section: Static function prototypes
 *
 * ============================================================================
 */

static void
clear_address_table(
    struct J1939AC* ac);

static void
cannot_claim_address(
    struct J1939AC* ac);

/* ============================================================================
 *
 * Section: Function definitions
 *
 * ============================================================================
 */

void
j1939_ac_init(
    struct J1939AC* ac,
    int node_idx,
    struct J1939Name* name,
    J1939_AC_STARTUP_DELAY_250MS startup_delay,
    void* startup_delay_param)
{
    uint8_t source_address = j1939_get_source_address(ac->node_idx);

    ac->node_idx = node_idx;
    ac->cannot_claim_address = false;
    memcpy(&ac->name, name, sizeof(struct J1939Name));

    clear_address_table(ac);

    // Send address claimed message
    j1939_tx_helper(
        ac->node_idx,
        J1939_ADDRESS_CLAIMED_PGN,
        (uint8_t*)&ac->name,
        J1939_ADDRESS_CLAIMED_LEN,
        J1939_ADDR_GLOBAL,
        J1939_ADDRESS_CLAIMED_PRI);

    // On startup, nodes claiming addresses in the range 0-127 or 248-253 may
    //  begin their regular network activities immediately. Nodes claiming
    //  other addresses should wait 250ms to begin.
    if ((source_address > 127) && (source_address < 248))
        startup_delay(startup_delay_param);
}

bool
j1939_ac_update_address(
    struct J1939AC* ac)
{
    if (ac->addresses_available == 0)
        return false;

    uint8_t source_address = j1939_get_source_address(ac->node_idx);
    uint8_t new_address;

    for (int i = 1; i <= (J1939_AC_MAX_ADDRESSES - 1); ++i)
    {
        new_address = (source_address + i) % J1939_AC_MAX_ADDRESSES;

        if (ac->address_table[new_address] == 0)
        {
            j1939_set_source_address(ac->node_idx, new_address);
            return true;
        }
    }

    // This shouldn't happen but just in case
    ac->addresses_available = 0;
    return false;
}

void
j1939_ac_rx_address_claim(
    struct J1939AC* ac,
    struct J1939Msg* msg)
{
    uint8_t source_address = j1939_get_source_address(ac->node_idx);
    struct J1939Name* received_name = (struct J1939Name*)msg->data;

    // If there's an address contention, check the NAME of the other node.
    // If our NAME if higher priority (lower value), we keep our address.
    // Otherwise, attempt to claim another address.
    if (msg->src == source_address)
    {
        if (*((uint64_t*)received_name) < *((uint64_t*)&ac->name))
        {
            j1939_close_transport_protocol_connection(ac->node_idx);

            if (!j1939_ac_update_address(ac))
            {
                cannot_claim_address(ac);
                return;
            }
        }

        // Send address claimed message
        j1939_tx_helper(
            ac->node_idx,
            J1939_ADDRESS_CLAIMED_PGN,
            (uint8_t*)&ac->name,
            J1939_ADDRESS_CLAIMED_LEN,
            J1939_ADDR_GLOBAL,
            J1939_ADDRESS_CLAIMED_PRI);
    }

    if (msg->src < 254)
    {
        ac->address_table[msg->src] = 1;
        ac->addresses_available--;
    }
}

void
j1939_ac_rx_address_claim_request(
    struct J1939AC* ac)
{
    // Each node in the network should respond to the request for address claim.
    // This allows us to update our address table.
    clear_address_table(ac);

    // Send address claimed message
    j1939_tx_helper(
        ac->node_idx,
        J1939_ADDRESS_CLAIMED_PGN,
        (uint8_t*)&ac->name,
        J1939_ADDRESS_CLAIMED_LEN,
        J1939_ADDR_GLOBAL,
        J1939_ADDRESS_CLAIMED_PRI);
}

/* ============================================================================
 *
 * Section: Static function definitions
 *
 * ============================================================================
 */

static void
clear_address_table(
    struct J1939AC* ac)
{
    uint8_t source_address = j1939_get_source_address(ac->node_idx);

    memset(ac->address_table, 0, sizeof(ac->address_table));

    if (source_address < J1939_AC_MAX_ADDRESSES)
    {
        ac->address_table[source_address] = 1;
        ac->addresses_available = J1939_AC_MAX_ADDRESSES - 1;
    }
}

static void
cannot_claim_address(
    struct J1939AC* ac)
{
    j1939_set_source_address(ac->node_idx, J1939_ADDR_NULL);
    ac->cannot_claim_address = true;

    j1939_tx_helper(
        ac->node_idx,
        J1939_CANNOT_CLAIM_ADDRESS_PGN,
        (uint8_t*)&ac->name,
        J1939_CANNOT_CLAIM_ADDRESS_LEN,
        J1939_ADDR_GLOBAL,
        J1939_CANNOT_CLAIM_ADDRESS_PRI);
}
