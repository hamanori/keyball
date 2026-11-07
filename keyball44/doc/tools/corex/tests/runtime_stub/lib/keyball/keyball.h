#pragma once

#include "stub.h"

#define KEYBALL_SAFE_RANGE QK_KB

typedef struct {
    report_mouse_t last_mouse;
} keyball_t;

extern keyball_t keyball;

uint8_t keyball_get_cpi(void);
void keyball_set_cpi(uint8_t cpi);
void keyball_set_scroll_mode(bool enabled);
