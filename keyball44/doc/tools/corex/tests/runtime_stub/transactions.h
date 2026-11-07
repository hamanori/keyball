#pragma once

#include "stub.h"

void transaction_register_rpc(uint8_t id, void (*handler)(uint8_t, const void *, uint8_t, void *));
bool transaction_rpc_send(uint8_t id, uint8_t length, const void *data);
