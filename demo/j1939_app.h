#pragma once

#include "j1939.h"

void
j1939_app_init(
    struct J1939* node,
    struct J1939Name* name,
    uint8_t preferred_address,
    int tick_rate_ms,
    const char* device_name);
