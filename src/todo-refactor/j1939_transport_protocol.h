#pragma once

#include "j1939.h"
#include <stdint.h>

void j1939_tp_queue(struct J1939* node, struct J1939Msg* msg);
