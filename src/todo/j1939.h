#pragma once

#include "j1939_define.h"

#include <stdint.h>
#include <stdbool.h>

struct J1939CanFrame {
    uint32_t can_id;
    uint8_t data[8];
    uint8_t len;
};

struct J1939Msg {
    // This represents all the data associated with a complete J1939 message
    uint32_t pgn;
    uint8_t* data;
    uint16_t len;
    uint8_t src;
    uint8_t dst;
    uint8_t pri;
};

// Returns true if there is a new CAN message to be received, in which case, the message will be placed in the function argument
typedef bool (*J1939_PHYSICAL_RX)(struct J1939CanFrame*);

// Implemented by the user application. The J1939 layer will pass messages to this function for transmission on the CAN bus.
// Returns true if successful, returns false otherwise.
typedef bool (*J1939_PHYSICAL_TX)(struct J1939Msg*);

typedef void (*J1939_APP_RX)(struct J1939Msg*);

typedef void (*J1939_STARTUP_DELAY_250MS)(void*);

enum j1939_tp_connection_type {
    J1939_TP_NONE = 0,
    J1939_TP_BROADCAST,
    J1939_TP_P2P
};


struct J1939 {
    // This struct represents a single node on the CAN bus

    struct {
        uint8_t source_address;
        int tick_rate_ms;
        J1939_PHYSICAL_RX physical_rx;
        J1939_PHYSICAL_TX physical_tx;
        J1939_APP_RX app_rx;
    } priv;

    struct {
        uint8_t buf[J1939_TP_DATA_MAX];
        int timer_ms;
        // What kind of connection is active?
        enum j1939_tp_connection_type connection_type;
        // Are we the sender or receiver in this connection?
        bool sender;
        // This is the sequence number for the next TP.DT (whether sender or receiver)
        int next_seq;
        // How many more data bytes to send/receive?
        int bytes_rem;
        // The number of packets for this multi-byte message
        uint8_t num_packets;

        bool received_cts;

        // MESSAGE DATA
        // pgn of the multi-byte message
        uint32_t pgn;
        // length of data in bytes
        uint16_t len;
        // message destination
        uint8_t dst;
        // message priority
        uint8_t pri;
        // address of message source
        uint8_t src;
    } tp;

    struct {
        struct J1939Name name;
        // TODO: remove
        uint8_t preferred_address;
        //bool address_claimed;
        bool cannot_claim_address;

        J1939_STARTUP_DELAY_250MS startup_delay;
        void* startup_delay_param;

        int_fast8_t address_table[254];
        uint_fast8_t addresses_available;

        // 
        int startup_timer;
    } ac;
};

// initialize the J1939 node state variables. Set the tick_rate_ms value, which is how often the user application calls the update function.
bool
j1939_init(
    struct J1939* node,
    struct J1939Name* name,
    J1939_PHYSICAL_RX physical_rx,
    J1939_PHYSICAL_TX physical_tx,
    J1939_APP_RX app_rx,
    J1939_STARTUP_DELAY_250MS startup_delay,
    void* startup_delay_param,
    uint8_t preferred_address,
    int tick_rate_ms);

// This function should be called by the user application at the rate specified by tick_rate_ms. This 
void
j1939_update(
    struct J1939* node);

// The user application will create a j1939 msg and pass it to this function, which serves as the top-level interface for sending j1939 messages on the bus.
// TODO: return value?
void
j1939_tx(
    struct J1939* node,
    struct J1939Msg* msg);
