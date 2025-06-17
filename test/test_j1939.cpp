#include "test_j1939.hpp"

J1939 TestJ1939::node;

void TestJ1939::testRunStarting(Catch::TestRunInfo const& test_run_info)
{
    (void)j1939_init(&TestJ1939::node, 0, NULL, NULL, NULL);
}

CATCH_REGISTER_LISTENER(TestJ1939)
