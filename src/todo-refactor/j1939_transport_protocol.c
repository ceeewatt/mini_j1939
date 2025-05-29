#include "j1939.h"

#include <math.h>
#include <string.h>
#include <stdbool.h>

void j1939_tp_broadcast_update(struct J1939* node);
void j1939_tp_p2p_update(struct J1939* node);


static void send_bam(struct J1939* node, uint32_t pgn, uint16_t len)
{
    struct J1939Msg bam;
    #define CONTROL_BYTE 32
    uint8_t buf[8];

    node->tp.packages = (uint8_t)ceilf((float)len / 7.0f);
    node->tp.len = len;
    node->tp.seq = 1;

    // TODO: move
    node->tp.byte_rem = len;

    buf[0] = CONTROL_BYTE;
    buf[1] = (uint8_t)len;
    buf[2] = (uint8_t)(len >> 8);
    buf[3] = node->tp.packages;
    buf[4] = 0xFF;
    buf[5] = (uint8_t)pgn;
    buf[6] = (uint8_t)(pgn >> 8);
    buf[7] = (uint8_t)(pgn >> 16);

    // send message on physical bus
    node->physical_tx();
}




// Copy a multi-byte message to an internal buffer for non-blocking transmission
//  of multi-byte messages from the application.
void j1939_tp_queue(struct J1939* node, struct J1939Msg* msg)
{
    static uint8_t tp_buf[1785];
    memcpy(tp_buf, msg->data, msg->len);

    node->tp.sender = true;
    node->tp.next_seq = 1;

    if (msg->dst == 255) {
        node->tp.update = j1939_tp_broadcast_update;
        send_bam(node, msg->pgn, msg->len);
    }
    else {
        node->tp.update = j1939_tp_p2p_update;
    }
}

void j1939_tp_broadcast_update(struct J1939* node)
{
    if (node->tp.sender) {
        if (node->tp.timer_ms < 50) {
            return;
        }

        if (node->tp.seq <= node->tp.packages) {
            int len = (node->tp.len > 7) ? 7 : node->tp.len;
            send_dt(node->tp.seq++, node->tp.buf, len);
            node->tp.len -= len;
        }

        node->tp.timer_ms = 0;
    }
    else {
        if (node->tp.timer_ms >= 750) {
            // send connection abort
        }
    }

}


void send_dt(uint8_t seq, uint8_t* data, uint8_t len)
{
    struct J1939Msg dt;
    uint8_t buf[8];
    buf[0] = seq;
    int i;
    for (i = 1; i <= len; ++i) {
        buf[i] = data[i];
    }
    for (i = len + 1; i < 8; ++i) {
        buf[i] = 0xFF;
    }

    // send message on physical bus
}


void j1939_tp_p2p_update(struct J1939* node)
{

}

void j1939_tp_update(struct J1939* node)
{
    if (node->tp.update != NULL) {
        node->tp.timer_ms += node->tick_rate_ms;
        node->tp.update(node);
    }
}

void j1939_tp_process_cm(struct J1939 *node, struct J1939Msg *msg)
{
    switch (msg->data[0]) {
        case 32:
            // we're receiving BAM
            if (node->tp.update != NULL) {

            }
            break;
    }
}

static void rx_bam(struct J1939* node, struct J1939Msg* msg)
{
    if (node->tp.update != NULL) {
        // we're receiving 
    }
}

void j1939_tp_process_dt(struct J1939 *node, struct J1939Msg *msg)
{
    // If there's no connection currently open
    if (!node->tp.update) {
        return;
    }

    uint8_t seq = msg->data[0];

    // Check that we're receiving the expected sequence number
    if (seq != node->tp.next_seq++) {
        // TODO: abort
        return;
    }

    uint8_t* buf = node->tp.buf + ((seq - 1) * 7);

    for (int i = 0;
         i < (node->tp.byte_rem < 7 ? node->tp.byte_rem : 7);
         ++i)
    {
        *(buf + i) = msg->data[1 + i];
    }
}
