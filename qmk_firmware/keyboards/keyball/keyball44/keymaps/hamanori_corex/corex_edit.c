#include "corex_edit.h"
#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define pgm_read_word(p) (*(p))
#endif

#define MOD_CTRL 0x01u
#define MOD_SHIFT 0x02u
#define MOD_ALT 0x04u
#define MOD_GUI 0x08u

#define HID_V 0x19u
#define HID_A 0x04u
#define HID_C 0x06u
#define HID_F 0x09u
#define HID_H 0x0Bu
#define HID_X 0x1Bu
#define HID_1 0x1Eu
#define HID_9 0x26u
#define HID_LEFT 0x50u
#define HID_RIGHT 0x4Fu
#define HID_UP 0x52u
#define HID_DOWN 0x51u
#define HID_HOME 0x4Au
#define HID_END 0x4Du

enum {
    ID_CLIPBOARD_HISTORY = 24,
    ID_REPLACE = 25,
    ID_CUT_ALL = 52,
    ID_COPY_ALL = 53,
    ID_CUT_LINE = 54,
    ID_COPY_LINE = 55,
    ID_SELECT_PARAGRAPH = 56,
    ID_SELECT_LINE = 57,
    ID_SELECT_WORD = 58,
    ID_LINE_HEAD = 59,
    ID_LINE_END = 60,
    ID_PASTE = 61,
    ID_FIRST_TAB = 62,
    ID_LAST_TAB = 63,
};

#define DOUBLE_TAP_MS 180u

static struct {
    bool valid;
    bool mac;
} observed_host;

static struct {
    bool pending;
    uint8_t id;
    bool mac;
    bool shifted;
    bool released;
    uint16_t released_at;
    uint8_t down_mask;
} boundary;

static struct {
    bool pending;
    bool down;
    bool mac;
    bool released;
    uint16_t released_at;
} paste;

static void emit(uint8_t key, uint8_t modifiers) {
    corex_edit_emit((uint16_t)(((uint16_t)modifiers << 8) | key));
}

static uint8_t primary_modifier(bool mac) {
    return mac ? MOD_GUI : MOD_CTRL;
}

static uint8_t word_modifier(bool mac) {
    return mac ? MOD_ALT : MOD_CTRL;
}

static void emit_line_head(bool mac, bool shifted) {
    uint8_t key = mac ? HID_LEFT : HID_HOME;
    uint8_t modifiers = mac ? MOD_GUI : 0;
    emit(key, (uint8_t)(modifiers | (shifted ? MOD_SHIFT : 0)));
}

static void emit_line_end(bool mac, bool shifted) {
    uint8_t key = mac ? HID_RIGHT : HID_END;
    uint8_t modifiers = mac ? MOD_GUI : 0;
    emit(key, (uint8_t)(modifiers | (shifted ? MOD_SHIFT : 0)));
}

static bool is_edit_id(uint8_t id) {
    return id == ID_CLIPBOARD_HISTORY || id == ID_REPLACE ||
           (id >= ID_CUT_ALL && id <= ID_LAST_TAB);
}

/* Clipboard, replace, cut/copy all, cut/copy line, paragraph/line/word, first/last tab. */
static const uint16_t PROGMEM fixed_actions[2][11][3] = {
    {{0x0819}, {0x010B}, {0x0104, 0x011B}, {0x0104, 0x0106},
     {0x004A, 0x024D, 0x011B}, {0x004A, 0x024D, 0x0106},
     {0x0151, 0x0352}, {0x004A, 0x024D}, {0x014F, 0x0350}, {0x011E}, {0x0126}},
    {{0x0A19}, {0x0C09}, {0x0804, 0x081B}, {0x0804, 0x0806},
     {0x0850, 0x0A4F, 0x081B}, {0x0850, 0x0A4F, 0x0806},
     {0x0451, 0x0652}, {0x0850, 0x0A4F}, {0x044F, 0x0650}, {0x081E}, {0x0826}}
};

static void emit_fixed_action(uint8_t id, bool mac) {
    uint8_t index = id <= ID_REPLACE ? id - ID_CLIPBOARD_HISTORY :
                    id <= ID_SELECT_WORD ? id - ID_CUT_ALL + 2 : id - ID_FIRST_TAB + 9;
    for (uint8_t i = 0; i < 3; i++) {
        uint16_t chord = pgm_read_word(&fixed_actions[mac][index][i]);
        if (!chord) break;
        corex_edit_emit(chord);
    }
}

static bool elapsed_at_least(uint16_t now, uint16_t then, uint16_t duration) {
    return (uint16_t)(now - then) >= duration;
}

static void clear_boundary(void) {
    boundary.pending = false;
}

static void observe_host(bool mac) {
    if (observed_host.valid && observed_host.mac != mac) {
        clear_boundary();
        boundary.down_mask = 0;
        paste.pending = false;
        paste.down = false;
    }
    observed_host.valid = true;
    observed_host.mac = mac;
}

static void emit_boundary(uint8_t id, bool mac, bool shifted, bool second) {
    if (second) {
        uint8_t key = id == ID_LINE_HEAD ? HID_UP : HID_DOWN;
        emit(key, (uint8_t)(word_modifier(mac) | (shifted ? MOD_SHIFT : 0)));
    } else if (id == ID_LINE_HEAD) {
        emit_line_head(mac, shifted);
    } else {
        emit_line_end(mac, shifted);
    }
}

static void emit_paste(bool mac, bool formatted) {
    uint8_t modifiers = primary_modifier(mac);
    if (formatted) {
        modifiers = (uint8_t)(modifiers | MOD_SHIFT | (mac ? MOD_ALT : 0));
    }
    emit(HID_V, modifiers);
}

static void flush_paste(void) {
    if (paste.pending) {
        emit_paste(paste.mac, false);
        paste.pending = false;
    }
}

static void process_paste(bool pressed, bool mac, uint16_t now) {
    if (!pressed) {
        if (paste.down) {
            paste.down = false;
            if (paste.pending) {
                paste.released = true;
                paste.released_at = now;
            }
        }
        return;
    }

    if (paste.down) {
        return;
    }
    if (paste.pending && paste.released && paste.mac == mac &&
        !elapsed_at_least(now, paste.released_at, DOUBLE_TAP_MS + 1u)) {
        paste.pending = false;
        paste.down = true;
        emit_paste(mac, true);
        return;
    }

    flush_paste();
    paste.pending = true;
    paste.down = true;
    paste.mac = mac;
    paste.released = false;
}

bool corex_edit_process(uint8_t id, bool pressed, bool mac, bool shifted, uint16_t now) {
    observe_host(mac);
    if (!is_edit_id(id)) {
        flush_paste();
        clear_boundary();
        return false;
    }
    if (id != ID_PASTE) {
        flush_paste();
    }
    if (id == ID_PASTE) {
        clear_boundary();
        process_paste(pressed, mac, now);
        return true;
    }
    if (id == ID_LINE_HEAD || id == ID_LINE_END) {
        uint8_t bit = id == ID_LINE_HEAD ? 1u : 2u;
        if (pressed) {
            if ((boundary.down_mask & bit) != 0) {
                return true;
            }
            boundary.down_mask |= bit;
            bool second = boundary.pending && boundary.released &&
                          boundary.id == id && boundary.mac == mac &&
                          boundary.shifted == shifted &&
                          !elapsed_at_least(now, boundary.released_at, DOUBLE_TAP_MS + 1u);
            if (second) {
                clear_boundary();
            } else {
                boundary.pending = true;
                boundary.id = id;
                boundary.mac = mac;
                boundary.shifted = shifted;
                boundary.released = false;
            }
            emit_boundary(id, mac, shifted, second);
        } else {
            boundary.down_mask &= (uint8_t)~bit;
            if (boundary.pending && boundary.id == id) {
                boundary.released = true;
                boundary.released_at = now;
            }
        }
        return true;
    }
    clear_boundary();
    if (pressed) {
        emit_fixed_action(id, mac);
    }
    return true;
}

void corex_edit_tick(bool mac, uint16_t now) {
    observe_host(mac);
    if (paste.pending && paste.released &&
        elapsed_at_least(now, paste.released_at, DOUBLE_TAP_MS)) {
        flush_paste();
    }
    if (boundary.pending && boundary.released &&
        elapsed_at_least(now, boundary.released_at, DOUBLE_TAP_MS)) {
        clear_boundary();
    }
}

void corex_edit_cancel(void) {
    observed_host.valid = false;
    boundary.pending = false;
    boundary.down_mask = 0;
    paste.pending = false;
    paste.down = false;
}
