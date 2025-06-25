#pragma once

/* ============================================================================
 * File: j1939_define.h
 *
 * Description: "Public" J1939 definitions.
 * ============================================================================
 */

#include <stdint.h>

/* ============================================================================
 *
 * Section: Macros
 *
 * ============================================================================
 */

#define J1939_ADDR_GLOBAL  (255)
#define J1939_ADDR_NULL    (254)

#define J1939_TP_MAX_PAYLOAD  (255 * 7)

#define J1939_DEFAULT_PRIORITY  (6)

#define J1939_TP_CM_CONTROL_BYTE_RTS  (16)
#define J1939_TP_CM_CONTROL_BYTE_CTS  (17)
#define J1939_TP_CM_CONTROL_BYTE_ACK  (19)
#define J1939_TP_CM_CONTROL_BYTE_BAM  (32)
#define J1939_TP_CM_CONTROL_BYTE_ABORT  (255)

#define J1939_TP_TIMEOUT_TR  (200)
#define J1939_TP_TIMEOUT_TH  (500)
#define J1939_TP_TIMEOUT_T1  (750)
#define J1939_TP_TIMEOUT_T2  (1250)
#define J1939_TP_TIMEOUT_T3  (1250)
#define J1939_TP_TIMEOUT_T4  (1050)

/* ============================================================================
 *
 * Section: Type definitions
 *
 * ============================================================================
 */

struct __attribute__((packed)) J1939_REQUEST {
    uint32_t pgn : 24;
};
#define J1939_REQUEST_PGN  (0x00EA00)
#define J1939_REQUEST_LEN  (3)
#define J1939_REQUEST_PRI  (6)

struct __attribute__((packed)) J1939Name {
    uint32_t identity : 21;
    uint16_t manufacturer : 11;
    uint8_t ecu_instance : 3;
    uint8_t function_instance : 5;
    uint8_t function : 8;
    uint8_t res : 1;
    uint8_t vehicle_system : 7;
    uint8_t vehicle_system_instance : 4;
    uint8_t industry_group : 3;
    uint8_t arbitrary_addr_capable : 1;
};

typedef struct J1939Name J1939_ADDRESS_CLAIMED;
#define J1939_ADDRESS_CLAIMED_PGN  (0x00EE00)
#define J1939_ADDRESS_CLAIMED_LEN  (8)
#define J1939_ADDRESS_CLAIMED_PRI  (6)

typedef J1939_ADDRESS_CLAIMED J1939_CANNOT_CLAIM_ADDRESS;
#define J1939_CANNOT_CLAIM_ADDRESS_PGN  (J1939_ADDRESS_CLAIMED_PGN)
#define J1939_CANNOT_CLAIM_ADDRESS_LEN  (J1939_ADDRESS_CLAIMED_LEN)
#define J1939_CANNOT_CLAIM_ADDRESS_PRI  (J1939_ADDRESS_CLAIMED_PRI)

// TODO: This is used to instruct a node with a given NAME to assume an address
struct __attribute__((packed)) J1939_COMMANDED_ADDRESS {
    struct J1939Name name;
    uint8_t new_source_address;
};
#define J1939_COMMANDED_ADDRESS_PGN  (0x00FED8)
#define J1939_COMMANDED_ADDRESS_LEN  (9)
#define J1939_COMMANDED_ADDRESS_PRI  (6)

struct __attribute__((packed)) J1939_TP_DT {
    uint8_t seq;
    uint8_t data0;
    uint8_t data1;
    uint8_t data2;
    uint8_t data3;
    uint8_t data4;
    uint8_t data5;
    uint8_t data6;
    uint8_t data7;
};
#define J1939_TP_DT_PGN  (0x00EB00)
#define J1939_TP_DT_LEN  (8)
#define J1939_TP_DT_PRI  (7)

struct __attribute__((packed)) J1939_TP_CM_RTS {
    uint8_t control_byte;
    uint16_t len;
    uint8_t num_packages;
    uint8_t max_packages;
    uint32_t pgn : 24;
};
struct __attribute__((packed)) J1939_TP_CM_CTS {
    uint8_t control_byte;
    uint8_t num_packages;
    uint8_t next_seq;
    uint16_t res;
    uint32_t pgn : 24;
};
struct __attribute__((packed)) J1939_TP_CM_ACK {
    uint8_t control_byte;
    uint16_t len;
    uint8_t num_packages;
    uint8_t res;
    uint32_t pgn : 24;
};
struct __attribute__((packed)) J1939_TP_CM_BAM {
    uint8_t control_byte;
    uint16_t len;
    uint8_t num_packages;
    uint8_t res;
    uint32_t pgn : 24;
};
struct __attribute__((packed)) J1939_TP_CM_ABORT {
    uint8_t control_byte;
    uint8_t abort_reason;
    uint32_t res : 24;
    uint32_t pgn : 24;
};
#define J1939_TP_CM_PGN  (0x00EC00)
#define J1939_TP_CM_LEN  (8)
#define J1939_TP_CM_PRI  (7)
