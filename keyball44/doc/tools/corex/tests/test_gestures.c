#include <assert.h>
#include <stdio.h>
#include "corex_gesture.h"

static uint16_t sent[32];
static unsigned count;
void corex_gesture_emit(uint16_t chord) { sent[count++] = chord; }
static void reset(void) { count = 0; corex_gesture_cancel(); }
static void stroke(uint8_t kind, bool mac, int16_t x, int16_t y, uint16_t now) {
    corex_gesture_key(kind, true, mac, now);
    assert(corex_gesture_motion(x, y, mac, now + 1));
    corex_gesture_key(kind, false, mac, now + 2);
}
int main(void) {
    reset();
    assert(!corex_gesture_motion(100, 0, true, 0));
    stroke(1, true, -90, 0, 10);
    assert(count == 1 && sent[0] == 0x0150); /* Ctrl+Left */
    reset();
    stroke(1, false, 0, -90, 10);
    assert(count == 1 && sent[0] == 0x082B); /* Win+Tab */
    reset();
    stroke(1, false, -90, 0, 10);
    assert(count == 1 && sent[0] == 0x0950); /* Ctrl+Win+Left */
    reset();
    stroke(2, true, 0, -90, 10);
    assert(count == 1 && sent[0] == 0x0D6D); /* Ctrl+Alt+Gui+F18 */
    reset();
    stroke(2, false, 0, -90, 10);
    assert(count == 1 && sent[0] == 0x0852); /* Win+Up, once */
    reset();
    stroke(3, false, 0, 90, 10);
    assert(count == 1 && sent[0] == 0x014E); /* Ctrl+PageDown */
    reset();
    stroke(4, false, 0, -90, 10);
    assert(count == 1 && sent[0] == 0x0133); /* JIS Ctrl+; */
    reset();
    corex_gesture_key(3, true, true, 0);
    corex_gesture_key(3, false, true, 30);
    corex_gesture_key(3, true, true, 100);
    corex_gesture_key(3, false, true, 130);
    assert(count == 1 && sent[0] == 0x0815); /* Gui+R */
    reset();
    corex_gesture_key(1, true, true, 0);
    corex_gesture_motion(90, 90, true, 1);
    assert(count == 0); /* ambiguous diagonal */
    corex_gesture_motion(90, 0, true, 2);
    assert(count == 1);
    corex_gesture_motion(100, 0, true, 100);
    assert(count == 1);
    corex_gesture_motion(100, 0, true, 302);
    assert(count == 2);
    corex_gesture_key(1, false, true, 310);
    assert(!corex_gesture_motion(100, 0, true, 500));
    reset();
    corex_gesture_key(1, true, true, 0);
    for (uint8_t i = 0; i < 20; i++) corex_gesture_motion(90, 90, true, i + 1);
    assert(count == 0);
    for (uint8_t i = 0; i < 20; i++) corex_gesture_motion(90, 0, true, i + 30);
    assert(count == 1 && sent[0] == 0x014F);
    reset();
    corex_gesture_key(1, true, true, 0);
    corex_gesture_motion(100, 0, false, 1);
    assert(count == 0); /* profile change cancels gesture */
    puts("gesture behavior: 11 cases passed");
}
