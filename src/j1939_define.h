#pragma once

#include <stdint.h>

#define J1939_ADDR_GLOBAL  (255)
#define J1939_ADDR_NULL    (254)

// The max number of data bytes to a multi-byte packet
#define J1939_TP_DATA_MAX  (255 * 7)

#define J1939_TP_CM_RTS_CONTROL_BYTE  (16)
#define J1939_TP_CM_CTS_CONTROL_BYTE  (17)
#define J1939_TP_CM_ACK_CONTROL_BYTE  (19)
#define J1939_TP_CM_BAM_CONTROL_BYTE  (32)
#define J1939_TP_CM_ABORT_CONTROL_BYTE  (255)

#define J1939_TP_CM_RTS_MAX_PACKETS  (0xFF)

#define J1939_DEFAULT_PRIORITY  (7)
#define J1939_TP_CM_DEFAULT_PRIORITY  (7)
#define J1939_TP_DT_DEFAULT_PRIORITY  (7)
#define J1939_AC_DEFAULT_PRIORITY  (7)

#define J1939_TP_DT_PERIOD  (50)

#define J1939_TP_TIMEOUT_TR  (200)
#define J1939_TP_TIMEOUT_TH  (500)
#define J1939_TP_TIMEOUT_T1  (750)
#define J1939_TP_TIMEOUT_T2  (1250)
#define J1939_TP_TIMEOUT_T3  (1250)
#define J1939_TP_TIMEOUT_T4  (1050)

enum j1939_tp_cm_abort_reason {
    J1939_REASON_BUSY = 0,
    J1939_REASON_RESOURCES,
    J1939_REASON_TIMEOUT,
    J1939_REASON_OTHER
};

#define J1939_TP_DT_PGN  0x00EB00
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

#define J1939_TP_CM_PGN  0x00EC00
struct __attribute__((packed)) J1939_TP_CM_RTS {
    uint8_t control_byte;
    uint16_t len;
    uint8_t packets;
    uint8_t max_packets;
    uint32_t pgn : 24;
};


struct __attribute__((packed)) J1939_TP_CM_CTS {
    uint8_t control_byte;
    uint8_t packets;
    uint8_t next_seq;
    uint16_t res;
    uint32_t pgn : 24;
};

struct __attribute__((packed)) J1939_TP_CM_ACK {
    uint8_t control_byte;
    uint16_t len;
    uint8_t packets;
    uint8_t res;
    uint32_t pgn : 24;
};

struct __attribute__((packed)) J1939_TP_CM_BAM {
    uint8_t control_byte;
    uint16_t len;
    uint8_t packets;
    uint8_t res;
    uint32_t pgn : 24;
};

struct __attribute__((packed)) J1939_TP_CM_ABORT {
    uint8_t control_byte;
    uint8_t abort_reason;
    uint32_t res : 24;
    uint32_t pgn : 24;
};

#define J1939_REQUEST_PGN  (0x00EA00)
struct __attribute__((packed)) J1939_REQUEST {
    uint32_t pgn : 24;
};

#define J1939_ADDRESS_CLAIMED_PGN  (0x00EE00)
struct __attribute__((packed)) J1939_ADDRESS_CLAIMED {
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

#define J1939_COMMANDED_ADDRESS_PGN  (0x00FED8)
struct __attribute__((packed)) J1939_COMMAND_ADDRESS {
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
    uint8_t source_address : 8;
};
