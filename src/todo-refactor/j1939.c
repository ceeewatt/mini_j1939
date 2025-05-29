#include "j1939.h"

#include <stdio.h>


void j1939_process(struct J1939* this, struct J1939Msg* msg)
{
    switch (msg->pgn) {
        // cases:
        //  address claim
        //  request
        //  transport protocol
        // else:
        //  pass to application
        case J1939_PGN_TP_CM:
            break;
        case J1939_PGN_TP_DT:
            j1939_tp_process_dt(this, msg);
            break;
    }
}

void j1939_rx(
    struct J1939* this,
    struct J1939Msg* msg,
    uint32_t can_id,
    uint8_t* data,
    uint8_t len)
{
    // Don't handle CAN 2.0A frames
    if (!((can_id >> 31) & 1)) {
        printf("CAN 2.0A frame!!\n");
        return;
    }

    uint8_t priority = (can_id >> 26) & 0x07;
    uint8_t dp = (can_id >> 24) & 1;
    uint8_t pf = (can_id >> 16) & 0xFF;
    uint8_t ps = (can_id >> 8) & 0xFF;
    uint8_t sa = can_id & 0xFF;
    uint32_t pgn;

    if (pf < 240) {
        // Destination-specific message: only handle message if addressed
        //  to us.
        if ((ps != J1939_GLOBAL_ADDRESS) && (ps != this->source_address)) {
            return;
        }

        pgn = (dp << 16) | (pf << 8);
        msg->dst = ps;
    }
    else {
        pgn = (dp << 16) | (pf << 8) | ps;
        msg->dst = J1939_GLOBAL_ADDRESS;
    }

    msg->pgn = pgn;
    msg->len = len;
    msg->src = sa;
    msg->pri = priority;

    for (int i = 0; i < len; ++i) {
        msg->data[i] = data[i];
    }

    j1939_process(this, msg);

    //printf("priority: %d\n", priority);
    //printf("dp: 0x%2X\n", dp);
    //printf("pf: 0x%2X\n", pf);
    //printf("ps: 0x%2X\n", ps);
    //printf("sa: 0x%2X\n", sa);
    //printf("pgn: 0x%2X\n", pgn);
}

void j1939_tx(struct J1939* this, struct J1939Msg* msg)
{
    if (msg->len <= 8) {
        // transmit single message
    }
    else {
        // transport protocol
        j1939_tp_queue(this, msg);
    }
}

