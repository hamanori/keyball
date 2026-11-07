#ifndef COREX_EDIT_H
#define COREX_EDIT_H

#include <stdbool.h>
#include <stdint.h>

bool corex_edit_process(uint8_t id, bool pressed, bool mac, bool shifted, uint16_t now);
void corex_edit_tick(bool mac, uint16_t now);
void corex_edit_cancel(void);

extern void corex_edit_emit(uint16_t chord);

#endif
