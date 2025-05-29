#include "j1939.h"

#include <stdint.h>

#define NODE1_SRC_ADDR  (0x55)
#define NODE2_SRC_ADDR  (0x56)

#define NODE1_IDENTITY  (0x1)
#define NODE2_IDENTITY  (0x2)

#define DUMMY1_PGN   (0x1200)
struct __attribute__((packed)) dummy1 {
    uint8_t a;
    uint8_t b;
    uint8_t c;
};

#define DUMMY2_PGN   (0xFE34)
struct __attribute__((packed)) dummy2 {
    uint8_t a;
    uint16_t b;
    uint32_t c;
    uint8_t d;
};

#define DUMMY3_PGN   (0xF012)
struct __attribute__((packed)) dummy3 {
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    uint8_t b4;
    uint8_t b5;
    uint8_t b6;
    uint8_t b7;
    uint8_t b8;
};

void node_init(struct J1939* node, struct J1939Name* name, uint8_t src_addr);
void node_superloop(struct J1939* node);
void node_tx_1hz(struct J1939* node);
