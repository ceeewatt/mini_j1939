#include "j1939_address_claim.h"
#include "j1939_private.h"

void
j1939_ac_init(
    struct J1939AC* ac,
    int node_idx)
{
    ac->node_idx = node_idx;
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
