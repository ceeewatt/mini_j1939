#include "j1939.h"
#include <string.h>

static void
tx_address_claim(
    struct J1939* node)
{
    struct J1939Msg msg;
    msg.pgn = J1939_ADDRESS_CLAIMED_PGN;
    msg.data = (uint8_t*)&node->ac.name;
    msg.len = 8;
    msg.src = node->priv.source_address;
    msg.dst = J1939_ADDR_GLOBAL;
    msg.pri = J1939_AC_DEFAULT_PRIORITY;

    j1939_tx(node, &msg);
}

static bool
update_address(
    struct J1939* node)
{
    if (node->ac.addresses_available == 0)
        return false;

    // Begin ascending the address table starting from our current address
    uint_fast8_t new_address;
    for (int i = 1; i <= (254 - 1); ++i)
    {
        new_address = (node->priv.source_address + i) % 254;

        if (node->ac.address_table[new_address] == 0)
        {
            node->priv.source_address = (uint8_t)new_address;
            return true;
        }
    }

    return false;
}

static void
clear_address_table(
    struct J1939* node)
{
    memset(node->ac.address_table, 0, sizeof(node->ac.address_table));
    node->ac.address_table[node->priv.source_address] = 1;
    node->ac.addresses_available = 253;
}

void
j1939_ac_rx_address_claim_request(
    struct J1939* node,
    struct J1939Msg* msg)
{
    // Each node in the network should respond to the request for address claim.
    // This allows us to update our address table.
    clear_address_table(node);
    tx_address_claim(node);
}

void
j1939_ac_rx_address_claim(
    struct J1939* node,
    struct J1939Msg* msg)
{
    struct J1939Name* name = (struct J1939Name*)msg->data;

    // If there's an address contention, check the NAME of the other node.
    // If our NAME if higher priority (lower value), we keep our address.
    // Otherwise, attempt to claim another address.
    if (msg->src == node->priv.source_address)
    {
        if (*((uint64_t*)name) < *((uint64_t*)&node->ac.name))
        {
            if (!update_address(node))
                node->ac.cannot_claim_address = true;

            tx_address_claim(node);

            // TODO: close any presently open TP connection
        }
        else
        {
            tx_address_claim(node);
        }
    }

    if (msg->src < 254)
    {
        node->ac.address_table[msg->src] = 1;
        node->ac.addresses_available--;
    }
}

void
j1939_ac_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t preferred_address,
    J1939_STARTUP_DELAY_250MS startup_delay,
    void* startup_delay_param)
{
    node->ac.startup_delay = startup_delay;
    node->ac.startup_delay_param = startup_delay_param;
    node->priv.source_address = preferred_address;

    clear_address_table(node);

    memcpy(&node->ac.name, name, sizeof(struct J1939Name));
    node->ac.cannot_claim_address = false;

    tx_address_claim(node);

    if ((node->priv.source_address > 127) && (node->priv.source_address < 248))
        node->ac.startup_delay(node->ac.startup_delay_param);
}
