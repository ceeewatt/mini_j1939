#include "test_j1939.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("New address search", "[j1939_ac_update_address]")
{
    J1939AC* ac = &g_j1939[TestJ1939::node.node_idx].ac;
    uint8_t original_address = g_j1939[TestJ1939::node.node_idx].j1939_public->source_address;

    SECTION("Function returns false if there are addresses available")
    {
        ac->addresses_available = 0;
        REQUIRE(j1939_ac_update_address(ac) == false);
    }
    SECTION("Function returns false if address table is exhausted")
    {
        // It shouldn't be possible for addresses_available to be a positive
        //  number and address_table be full.
        ac->addresses_available = 1;
        std::memset(ac->address_table, 1, sizeof(ac->address_table));

        REQUIRE(j1939_ac_update_address(ac) == false);
        REQUIRE(ac->addresses_available == 0);
    }
    SECTION("Function loops over entire address_table until it finds and available address")
    {
        ac->addresses_available = 1;
        std::memset(ac->address_table, 1, sizeof(ac->address_table));

        TestJ1939::node.source_address = 80;
        ac->address_table[79] = 0;

        REQUIRE(j1939_ac_update_address(ac) == true);
        REQUIRE(TestJ1939::node.source_address == 79);
    }
    SECTION("Loop wraps around to beginning of address_table during search")
    {
        ac->addresses_available = 1;
        std::memset(ac->address_table, 1, sizeof(ac->address_table));

        TestJ1939::node.source_address = 80;

        const int foo = 85;
        ac->address_table[foo] = 0;

        REQUIRE(j1939_ac_update_address(ac) == true);
        REQUIRE(TestJ1939::node.source_address == foo);
    }

    g_j1939[TestJ1939::node.node_idx].j1939_public->source_address = original_address;
}

TEST_CASE("Recieving address claim", "[j1939_ac_rx_address_claim]")
{
    J1939AC* ac = &g_j1939[TestJ1939::node.node_idx].ac;
    uint8_t original_address = g_j1939[TestJ1939::node.node_idx].j1939_public->source_address;

    const uint8_t our_address = 0x55;
    g_j1939[TestJ1939::node.node_idx].j1939_public->source_address = our_address;

    J1939_ADDRESS_CLAIMED received_ac;
    J1939Msg received_msg {
        .pgn = J1939_ADDRESS_CLAIMED_PGN,
        .data = (uint8_t*)&received_ac,
        .len = J1939_ADDRESS_CLAIMED_LEN,
        .src = 0,
        .dst = J1939_ADDR_GLOBAL,
        .pri = J1939_ADDRESS_CLAIMED_PRI
    };

    SECTION("Address table is updated accordingly")
    {
        std::memset(ac->address_table, 0, sizeof(ac->address_table));
        ac->addresses_available = J1939_AC_MAX_ADDRESSES;

        received_msg.src = our_address + 1;
        j1939_ac_rx_address_claim(ac, &received_msg);

        REQUIRE(ac->address_table[received_msg.src] == 1);
        REQUIRE(ac->addresses_available == (J1939_AC_MAX_ADDRESSES - 1));
    }
    SECTION("Receive lower priority NAME")
    {
        // Create a NAME that is lower priority (higher numerical value) that ours
        std::memset(&received_ac, 0xFF, sizeof(J1939Name));
        received_msg.src = our_address;

        j1939_ac_rx_address_claim(ac, &received_msg);

        REQUIRE(TestJ1939::msg.pgn == J1939_ADDRESS_CLAIMED_PGN);
        REQUIRE(TestJ1939::msg.len == J1939_ADDRESS_CLAIMED_LEN);
        REQUIRE(TestJ1939::msg.src == our_address);
        REQUIRE(TestJ1939::msg.dst == J1939_ADDR_GLOBAL);
        REQUIRE(TestJ1939::msg.pri == J1939_ADDRESS_CLAIMED_PRI);
        REQUIRE(std::memcmp(TestJ1939::msg.data, &TestJ1939::name, sizeof(J1939Name)) == 0);
    }
    SECTION("Receive higher priority NAME and can claim a new address")
    {
        // Create a NAME that is higher priority (lower numerical value) that ours
        std::memset(&received_ac, 0x00, sizeof(J1939Name));
        received_msg.src = our_address;

        std::memset(ac->address_table, 0, sizeof(ac->address_table));
        ac->address_table[our_address] = 1;
        ac->addresses_available = J1939_AC_MAX_ADDRESSES - 1;

        j1939_ac_rx_address_claim(ac, &received_msg);

        // We should sent out an address claim msg with our newly claimed addr
        REQUIRE(TestJ1939::msg.src == (our_address + 1));
        REQUIRE(TestJ1939::msg.pgn == J1939_ADDRESS_CLAIMED_PGN);
        REQUIRE(TestJ1939::msg.len == J1939_ADDRESS_CLAIMED_LEN);
        REQUIRE(TestJ1939::msg.dst == J1939_ADDR_GLOBAL);
        REQUIRE(TestJ1939::msg.pri == J1939_ADDRESS_CLAIMED_PRI);
        REQUIRE(std::memcmp(TestJ1939::msg.data, &TestJ1939::name, sizeof(J1939Name)) == 0);
    }
    SECTION("Receive higher priority NAME and cannot claim a new address")
    {
        // Create a NAME that is higher priority (lower numerical value) that ours
        std::memset(&received_ac, 0x00, sizeof(J1939Name));
        received_msg.src = our_address;

        J1939TP* tp = &g_j1939[ac->node_idx].tp;

        ac->addresses_available = 0;
        tp->connection = J1939_TP_CONNECTION_BROADCAST;
        j1939_ac_rx_address_claim(ac, &received_msg);

        // Any open TP connections should be closed
        REQUIRE(g_j1939[ac->node_idx].tp.connection == J1939_TP_CONNECTION_NONE);

        // We should send out an CANNOT CLAIM ADDRESS message and set our source
        //  address to 254.
        REQUIRE(TestJ1939::msg.src == J1939_ADDR_NULL);
        REQUIRE(TestJ1939::msg.pgn == J1939_CANNOT_CLAIM_ADDRESS_PGN);
        REQUIRE(TestJ1939::msg.len == J1939_CANNOT_CLAIM_ADDRESS_LEN);
        REQUIRE(TestJ1939::msg.dst == J1939_ADDR_GLOBAL);
        REQUIRE(TestJ1939::msg.pri == J1939_CANNOT_CLAIM_ADDRESS_PRI);
        REQUIRE(std::memcmp(TestJ1939::msg.data, &TestJ1939::name, sizeof(J1939Name)) == 0);
    }

    g_j1939[TestJ1939::node.node_idx].j1939_public->source_address = original_address;
}
