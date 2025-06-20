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
}
