#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Kinds: screen=1, window=2, tab=3, zoom=4. */
void corex_gesture_key(uint8_t kind, bool pressed, bool mac, uint16_t now);
bool corex_gesture_motion(int8_t dx, int8_t dy, bool mac, uint16_t now);
void corex_gesture_cancel(void);
void corex_gesture_sensitivity(int8_t step);
void corex_gesture_emit(uint16_t chord);
