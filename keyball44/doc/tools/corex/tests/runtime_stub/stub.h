#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <keycodes.h>
#include <modifiers.h>
#include <quantum_keycodes.h>

#define USER_OLED_SYNC 1
#include "config.h"

#define PROGMEM
#define PSTR(text) (text)
#define pgm_read_byte(address) (*(const uint8_t *)(address))
#define pgm_read_word(address) (*(const uint16_t *)(address))
#define MATRIX_ROWS 8
#define MATRIX_COLS 6
#define COMBO_END 0
#define COMBO_ACTION(keys) { (keys) }

typedef uint16_t layer_state_t;

typedef struct {
    uint8_t row;
    uint8_t col;
} keypos_t;

typedef struct {
    bool pressed;
    keypos_t key;
} keyevent_t;

typedef struct {
    uint8_t count;
} tap_t;

typedef struct {
    keyevent_t event;
    tap_t tap;
} keyrecord_t;

typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t v;
    int8_t h;
} report_mouse_t;

typedef struct {
    const uint16_t *keys;
} combo_t;

typedef struct {
    uint16_t keycode;
    uint8_t modifiers;
    bool pressed;
} stub_key_event_t;

typedef struct {
    uint8_t button_keycode;
    bool pressed;
    uint8_t buttons_after;
} stub_mouse_event_t;

#define STUB_EVENT_CAPACITY 256

extern layer_state_t layer_state;
extern layer_state_t default_layer_state;
extern uint8_t stub_modifiers;
extern uint8_t stub_weak_modifiers;
extern uint32_t stub_now;
extern uint8_t stub_mouse_buttons;
extern size_t stub_key_event_count;
extern size_t stub_tap_event_count;
extern size_t stub_mouse_event_count;
extern stub_key_event_t stub_key_events[STUB_EVENT_CAPACITY];
extern stub_key_event_t stub_tap_events[STUB_EVENT_CAPACITY];
extern stub_mouse_event_t stub_mouse_events[STUB_EVENT_CAPACITY];

uint8_t get_mods(void);
uint8_t get_weak_mods(void);
void set_mods(uint8_t mods);
void add_mods(uint8_t mods);
void del_mods(uint8_t mods);
void clear_mods(void);
void set_weak_mods(uint8_t mods);
void add_weak_mods(uint8_t mods);
void del_weak_mods(uint8_t mods);
void clear_weak_mods(void);
void send_keyboard_report(void);
void tap_code16(uint16_t keycode);
void tap_code(uint16_t keycode);
void register_code16(uint16_t keycode);
void unregister_code16(uint16_t keycode);
void clear_keyboard(void);
void register_mouse(uint8_t button_keycode, bool pressed);
void wait_ms(uint16_t ms);
uint16_t timer_read(void);
uint32_t timer_read32(void);
uint16_t timer_elapsed(uint16_t then);
uint32_t timer_elapsed32(uint32_t then);
void layer_on(uint8_t layer);
void layer_off(uint8_t layer);
void layer_clear(void);
void default_layer_set(layer_state_t state);
bool layer_state_is(uint8_t layer);
uint8_t get_highest_layer(layer_state_t state);
void eeconfig_update_user(uint32_t value);
uint32_t eeconfig_read_user(void);
bool is_keyboard_master(void);

layer_state_t layer_state_set_user(layer_state_t state);
bool pre_process_record_user(uint16_t keycode, keyrecord_t *record);
bool process_record_user(uint16_t keycode, keyrecord_t *record);
void post_process_record_user(uint16_t keycode, keyrecord_t *record);
void housekeeping_task_user(void);
report_mouse_t pointing_device_task_user(report_mouse_t report);

void stub_clear_events(void);
