#pragma once

#include <stdint.h>
#include <stdbool.h>

#define J1939_GLOBAL_ADDRESS 255

#define J1939_PGN_TP_CM 60416
#define J1939_PGN_TP_DT 60160


struct J1939 {
    uint8_t source_address;
    void (*physical_tx)(void);

    uint_fast8_t tick_rate_ms;

    struct {
        uint8_t buf[1785];
        uint_fast16_t timer_ms;
        void (*update)(struct J1939* node);
        bool sender;

        uint8_t packages;
        uint16_t len;
        uint8_t seq;

        uint8_t next_seq;
        // Byte remaining: keep track of how many data bytes have been received
        uint16_t byte_rem;
    } tp;
};

struct J1939Msg {
    uint32_t pgn;
    uint8_t* data;
    uint8_t len;
    uint8_t src;
    uint8_t dst;
    uint8_t pri;
};


void j1939_tp_process_cm(struct J1939* node, struct J1939Msg* msg);
void j1939_tp_process_dt(struct J1939* node, struct J1939Msg* msg);
