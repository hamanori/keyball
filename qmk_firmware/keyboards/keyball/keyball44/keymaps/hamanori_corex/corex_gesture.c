#include "corex_gesture.h"
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define pgm_read_word(p) (*(p))
#endif

/* up, down, left, right, double; Windows codes already use JIS for zoom. */
static const uint16_t PROGMEM actions[2][4][5] = {
    {{0x082B, 0x0807, 0x0950, 0x094F, 0x0807},
     {0x0852, 0x0851, 0x0850, 0x084F, 0x0851},
     {0x014B, 0x014E, 0x0450, 0x044F, 0x0115},
     {0x0133, 0x012D, 0x0127, 0x0127, 0x0127}},
    {{0x0152, 0x0151, 0x0150, 0x014F, 0x083C},
     {0x0D6D, 0x0D6C, 0x0068, 0x006E, 0x0D6B},
     {0x032B, 0x012B, 0x082F, 0x0830, 0x0815},
     {0x0A2E, 0x082D, 0x0827, 0x0827, 0x0827}}
};
static uint8_t active, last;
static bool host, moved, paused;
static int16_t x, y;
static uint16_t released, fired;
static uint8_t threshold = 80;

void corex_gesture_cancel(void) {
    active = last = 0;
    x = y = 0;
    moved = paused = false;
}

void corex_gesture_sensitivity(int8_t step) {
    if (step > 0 && threshold > 20) threshold -= 20;
    if (step < 0 && threshold < 200) threshold += 20;
}

void corex_gesture_key(uint8_t kind, bool pressed, bool mac, uint16_t now) {
    if (kind < 1 || kind > 4) return;
    if (pressed) {
        if (active) return;
        if (host != mac || (uint16_t)(now - released) > 180) last = 0;
        host = mac;
        active = kind;
        x = y = 0;
        moved = paused = false;
    } else if (active == kind) {
        if (!moved && host == mac) {
            if (last == kind && (uint16_t)(now - released) <= 180) {
                corex_gesture_emit(pgm_read_word(&actions[mac][kind - 1][4]));
                last = 0;
            } else {
                last = kind;
                released = now;
            }
        } else last = 0;
        active = 0;
    }
}

bool corex_gesture_motion(int8_t dx, int8_t dy, bool mac, uint16_t now) {
    if (!active) return false;
    if (host != mac) { corex_gesture_cancel(); return true; }
    if (paused && (uint16_t)(now - fired) < 300) return true;
    paused = false;
    /* Reports are signed 8-bit; scaling both axes retains direction after long diagonals. */
    x += dx; y += dy;
    if (x > 1000 || x < -1000 || y > 1000 || y < -1000) { x /= 2; y /= 2; }
    int16_t ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
    uint8_t direction;
    if (ax >= threshold && ax * 2 > ay * 3) direction = x < 0 ? 2 : 3;
    else if (ay >= threshold && ay * 2 > ax * 3) direction = y < 0 ? 0 : 1;
    else return true;
    corex_gesture_emit(pgm_read_word(&actions[mac][active - 1][direction]));
    x = y = 0;
    moved = paused = true;
    last = 0;
    fired = now;
    return true;
}
