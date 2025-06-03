#pragma once

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include <stdint.h>

extern "C" {
    struct CanIdConverter {
        uint8_t pri;
        uint8_t dp;
        uint8_t pf;
        uint8_t ps;
        uint8_t sa;
        uint32_t pgn;
    };

    #include "j1939_private.h"
    #include "j1939_transport_protocol_helper.h"
    #include "j1939_address_claim.h"

    extern struct J1939Private g_j1939[];
}

class TestJ1939 : public Catch::EventListenerBase {
public:
    static J1939 node;
    static J1939Msg msg;
    static uint8_t msg_buf[J1939_TP_MAX_PAYLOAD];
    static J1939Name name;

    static bool can_rx(J1939CanFrame* jframe);
    static bool can_tx(J1939Msg* msg);
    static void j1939_rx(J1939Msg* msg);

    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const& test_run_info) override;
};
