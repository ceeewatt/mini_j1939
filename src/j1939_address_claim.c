#include "j1939_address_claim.h"
#include "j1939_private.h"

#include <string.h>

static void
cannot_claim_address(
    struct J1939AC* ac)
{
    g_j1939[ac->node_idx].j1939_public->source_address = J1939_ADDR_NULL;
    ac->cannot_claim_address = true;

    j1939_tx_helper(
        ac->node_idx,
        J1939_CANNOT_CLAIM_ADDRESS_PGN,
        (uint8_t*)&ac->name,
        J1939_CANNOT_CLAIM_ADDRESS_LEN,
        J1939_ADDR_GLOBAL,
        J1939_CANNOT_CLAIM_ADDRESS_PRI);
}

void
j1939_ac_init(
    struct J1939AC* ac,
    int node_idx,
    struct J1939Name* name)
{
    ac->node_idx = node_idx;

    memcpy(&ac->name, name, sizeof(struct J1939Name));
}

bool
j1939_ac_update_address(
    struct J1939AC* ac)
{
    if (ac->addresses_available == 0)
        return false;

    struct J1939* node = g_j1939[ac->node_idx].j1939_public;
    uint8_t new_address;

    for (int i = 1; i <= (J1939_AC_MAX_ADDRESSES - 1); ++i)
    {
        new_address = (node->source_address + i) % J1939_AC_MAX_ADDRESSES;

        if (ac->address_table[new_address] == 0)
        {
            node->source_address = new_address;
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
    struct J1939Private* jp = &g_j1939[ac->node_idx];
    struct J1939Name* received_name = (struct J1939Name*)msg->data;

    // If there's an address contention, check the NAME of the other node.
    // If our NAME if higher priority (lower value), we keep our address.
    // Otherwise, attempt to claim another address.
    if (msg->src == jp->j1939_public->source_address)
    {
        if (*((uint64_t*)received_name) < *((uint64_t*)&ac->name))
        {
            j1939_tp_close_connection(&jp->tp);

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
