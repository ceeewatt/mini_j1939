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
