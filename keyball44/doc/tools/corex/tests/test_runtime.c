#include <stdio.h>
#include <string.h>

#include "stub.h"
#include "lib/keyball/keyball.h"
#include "corex_port.h"
#include "corex_edit.h"
#include "corex_gesture.h"

layer_state_t layer_state;
layer_state_t default_layer_state = 1;
uint8_t stub_modifiers;
uint8_t stub_weak_modifiers;
uint32_t stub_now;
uint8_t stub_mouse_buttons;
size_t stub_key_event_count;
size_t stub_tap_event_count;
size_t stub_mouse_event_count;
stub_key_event_t stub_key_events[STUB_EVENT_CAPACITY];
stub_key_event_t stub_tap_events[STUB_EVENT_CAPACITY];
stub_mouse_event_t stub_mouse_events[STUB_EVENT_CAPACITY];
keyball_t keyball;

static uint32_t stub_user_config;
static const char *current_test;
static unsigned current_failures;
static unsigned total_failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", current_test, __LINE__, #condition); \
        current_failures++; \
        total_failures++; \
    } \
} while (0)

uint8_t get_mods(void) { return stub_modifiers; }
uint8_t get_weak_mods(void) { return stub_weak_modifiers; }
void set_mods(uint8_t mods) { stub_modifiers = mods; }
void add_mods(uint8_t mods) { stub_modifiers |= mods; }
void del_mods(uint8_t mods) { stub_modifiers &= (uint8_t)~mods; }
void clear_mods(void) { stub_modifiers = 0; }
void set_weak_mods(uint8_t mods) { stub_weak_modifiers = mods; }
void add_weak_mods(uint8_t mods) { stub_weak_modifiers |= mods; }
void del_weak_mods(uint8_t mods) { stub_weak_modifiers &= (uint8_t)~mods; }
void clear_weak_mods(void) { stub_weak_modifiers = 0; }
void send_keyboard_report(void) {}

static void record_tap(uint16_t keycode) {
    if (stub_tap_event_count < STUB_EVENT_CAPACITY) {
        stub_tap_events[stub_tap_event_count++] = (stub_key_event_t){
            .keycode = keycode,
            .modifiers = (uint8_t)(stub_modifiers | stub_weak_modifiers),
            .pressed = true,
        };
    }
}

void tap_code16(uint16_t keycode) { record_tap(keycode); }
void tap_code(uint16_t keycode) { record_tap(keycode); }

static uint8_t modifier_for_key(uint16_t keycode) {
    switch (keycode) {
        case KC_LCTL: return MOD_BIT_LCTRL;
        case KC_LSFT: return MOD_BIT_LSHIFT;
        case KC_LALT: return MOD_BIT_LALT;
        case KC_LGUI: return MOD_BIT_LGUI;
        case KC_RCTL: return MOD_BIT_RCTRL;
        case KC_RSFT: return MOD_BIT_RSHIFT;
        case KC_RALT: return MOD_BIT_RALT;
        case KC_RGUI: return MOD_BIT_RGUI;
        default: return 0;
    }
}

static void record_key(uint16_t keycode, bool pressed) {
    if (stub_key_event_count < STUB_EVENT_CAPACITY) {
        stub_key_events[stub_key_event_count++] = (stub_key_event_t){
            .keycode = keycode,
            .modifiers = (uint8_t)(stub_modifiers | stub_weak_modifiers),
            .pressed = pressed,
        };
    }
}

void register_code16(uint16_t keycode) {
    uint8_t modifier = modifier_for_key(keycode);
    if (modifier) {
        add_mods(modifier);
    } else {
        record_key(keycode, true);
    }
}

void unregister_code16(uint16_t keycode) {
    uint8_t modifier = modifier_for_key(keycode);
    if (modifier) {
        del_mods(modifier);
    } else {
        record_key(keycode, false);
    }
}

void clear_keyboard(void) {
    stub_modifiers = 0;
    stub_weak_modifiers = 0;
}

void register_mouse(uint8_t button_keycode, bool pressed) {
    uint8_t bit = (uint8_t)(1u << (button_keycode - KC_MS_BTN1));
    if (pressed) {
        stub_mouse_buttons |= bit;
    } else {
        stub_mouse_buttons &= (uint8_t)~bit;
    }
    if (stub_mouse_event_count < STUB_EVENT_CAPACITY) {
        stub_mouse_events[stub_mouse_event_count++] = (stub_mouse_event_t){
            .button_keycode = button_keycode,
            .pressed = pressed,
            .buttons_after = stub_mouse_buttons,
        };
    }
}

void wait_ms(uint16_t ms) { stub_now += ms; }
uint16_t timer_read(void) { return (uint16_t)stub_now; }
uint32_t timer_read32(void) { return stub_now; }
uint16_t timer_elapsed(uint16_t then) { return (uint16_t)(timer_read() - then); }
uint32_t timer_elapsed32(uint32_t then) { return stub_now - then; }

void layer_on(uint8_t layer) {
    layer_state = layer_state_set_user((layer_state_t)(layer_state | ((layer_state_t)1u << layer)));
}

void layer_off(uint8_t layer) {
    layer_state = layer_state_set_user((layer_state_t)(layer_state & ~((layer_state_t)1u << layer)));
}

void layer_clear(void) { layer_state = layer_state_set_user(0); }
void default_layer_set(layer_state_t state) { default_layer_state = state; }
bool layer_state_is(uint8_t layer) { return (layer_state & ((layer_state_t)1u << layer)) != 0; }

uint8_t get_highest_layer(layer_state_t state) {
    for (int8_t layer = 15; layer >= 0; layer--) {
        if (state & ((layer_state_t)1u << layer)) return (uint8_t)layer;
    }
    return 0;
}

void eeconfig_update_user(uint32_t value) { stub_user_config = value; }
uint32_t eeconfig_read_user(void) { return stub_user_config; }
bool is_keyboard_master(void) { return true; }
void keyball_set_scroll_mode(bool enabled) { (void)enabled; }
uint8_t keyball_get_cpi(void) { return 70; }
void keyball_set_cpi(uint8_t cpi) { (void)cpi; }
void transaction_register_rpc(uint8_t id, void (*handler)(uint8_t, const void *, uint8_t, void *)) {
    (void)id; (void)handler;
}
bool transaction_rpc_send(uint8_t id, uint8_t length, const void *data) {
    (void)id; (void)length; (void)data; return true;
}

void stub_clear_events(void) {
    stub_key_event_count = 0;
    stub_tap_event_count = 0;
    stub_mouse_event_count = 0;
}

static keyrecord_t make_record(bool pressed, uint8_t row, uint8_t col, uint8_t tap_count) {
    keyrecord_t record = {0};
    record.event.pressed = pressed;
    record.event.key.row = row;
    record.event.key.col = col;
    record.tap.count = tap_count;
    return record;
}

/* Match QMK action.c: weak modifiers are cleared before keymap preprocess. */
static bool physical_key(uint16_t keycode, bool pressed, uint8_t row, uint8_t col, uint8_t tap_count) {
    if (pressed) clear_weak_mods();
    keyrecord_t record = make_record(pressed, row, col, tap_count);
    if (!pre_process_record_user(keycode, &record)) return false;
    bool continue_qmk = process_record_user(keycode, &record);
    if (continue_qmk) {
        if (IS_QK_LAYER_TAP(keycode) && tap_count == 0) {
            uint8_t layer = QK_LAYER_TAP_GET_LAYER(keycode);
            if (pressed) layer_on(layer); else layer_off(layer);
        } else if (IS_QK_MOD_TAP(keycode) && tap_count == 0) {
            /* QMK's hold action owns modifier registration. */
        } else if (pressed) {
            register_code16(keycode);
        } else {
            unregister_code16(keycode);
        }
        post_process_record_user(keycode, &record);
    }
    return continue_qmk;
}

static void reset_runtime(void) {
    keyrecord_t profile = make_record(true, 7, 0, 1);
    process_record_user(KC_PROFILE, &profile);
    process_record_user(KC_PROFILE, &profile);
    layer_clear();
    default_layer_set(1u << 0);
    clear_keyboard();
    corex_edit_cancel();
    corex_gesture_cancel();
    stub_mouse_buttons = 0;
    stub_now = 0;
    stub_user_config = 30;
    keyball.last_mouse = (report_mouse_t){0};
    stub_clear_events();
}

static void test_drag_lock_click_releases_on_keyup(void) {
    reset_runtime();
    physical_key(KC_DRAG_LOCK, true, 0, 0, 1);
    CHECK(stub_mouse_buttons == 1);
    physical_key(KC_MY_BTN1, true, 2, 4, 1);
    CHECK(stub_mouse_buttons == 1);
    physical_key(KC_MY_BTN1, false, 2, 4, 1);
    CHECK(stub_mouse_buttons == 0);
    CHECK(stub_mouse_event_count >= 3);
}

static void test_precision_survives_idle_and_scales_small_motion(void) {
    report_mouse_t report = {0};
    reset_runtime();
    report.x = 6;
    pointing_device_task_user(report);
    CHECK(layer_state_is(4)); /* Windows mouse layer. */
    physical_key(KC_PRECISION, true, 1, 1, 1);
    stub_now += 10001;
    housekeeping_task_user();
    CHECK(layer_state_is(4));

    report = (report_mouse_t){.x = 1};
    CHECK(pointing_device_task_user(report).x == 0);
    CHECK(pointing_device_task_user(report).x == 0);
    CHECK(pointing_device_task_user(report).x == 0);
    CHECK(pointing_device_task_user(report).x == 1);

    physical_key(KC_A, true, 0, 1, 1);
    physical_key(KC_A, false, 0, 1, 1);
    CHECK(pointing_device_task_user(report).x == 1); /* Typing clears precision. */

    report.x = 6;
    pointing_device_task_user(report);
    physical_key(KC_PRECISION, true, 1, 1, 1);
    physical_key(KC_PROFILE, true, 7, 1, 1);
    report.x = 1;
    CHECK(pointing_device_task_user(report).x == 1); /* Profile switch clears it too. */
}

static void test_window_switch_alt_survives_action_clear_and_releases_on_exit(void) {
    reset_runtime();
    layer_on(1); /* Windows edit layer. */
    physical_key(KC_WINDOW_SWITCH, true, 1, 2, 1);
    physical_key(KC_WINDOW_SWITCH, false, 1, 2, 1);
    CHECK((stub_weak_modifiers & MOD_BIT_LALT) != 0);

    physical_key(KC_LEFT, true, 2, 2, 1);
    CHECK(stub_key_event_count > 0);
    CHECK((stub_key_events[stub_key_event_count - 1].modifiers & MOD_BIT_LALT) != 0);
    physical_key(KC_LEFT, false, 2, 2, 1);

    set_mods(MOD_BIT_LALT); /* A real Alt key is physically held. */
    layer_off(1);
    CHECK((stub_weak_modifiers & MOD_BIT_LALT) == 0);
    CHECK((stub_modifiers & MOD_BIT_LALT) != 0);
}

static void test_mouse_k_hold_enters_numbers_without_click_or_typing(void) {
    reset_runtime();
    layer_on(4); /* Windows mouse layer. */
    uint16_t mouse_k = LT(3, KC_BTN3); /* Hold for numbers, tap for middle click. */
    physical_key(mouse_k, true, 4, 4, 0);
    physical_key(mouse_k, false, 4, 4, 0);
    CHECK(stub_mouse_event_count == 0);
    CHECK(stub_key_event_count == 0);
    CHECK(stub_tap_event_count == 0);
    CHECK(stub_mouse_buttons == 0);
}

static void test_new_gesture_does_not_inherit_drag_distance(void) {
    reset_runtime();
    report_mouse_t report = {.x = 6};
    pointing_device_task_user(report); /* Enter mouse layer. */
    physical_key(KC_MY_BTN1, true, 2, 3, 1);
    report.x = 8;
    pointing_device_task_user(report); /* Complete a drag above the threshold. */
    physical_key(KC_MY_BTN1, false, 2, 3, 1);
    CHECK(!layer_state_is(4)); /* The ball must settle before normal re-entry. */

    physical_key(KC_GESTURE_SCREEN, true, 2, 4, 1);
    physical_key(KC_GESTURE_SCREEN, false, 2, 4, 1);
    CHECK(layer_state_is(4)); /* New gesture release should enter double-click grace. */
}

static void test_profile_switch_releases_held_operations(void) {
    reset_runtime();
    physical_key(KC_MY_BTN1, true, 2, 3, 1);
    physical_key(KC_GESTURE_SCREEN, true, 2, 4, 1);
    CHECK(stub_mouse_buttons == 1);

    physical_key(KC_PROFILE, true, 7, 1, 1);
    CHECK(stub_mouse_buttons == 0);
    size_t taps_after_switch = stub_tap_event_count;
    report_mouse_t movement = {.x = 100};
    pointing_device_task_user(movement);
    CHECK(stub_tap_event_count == taps_after_switch);
    physical_key(KC_MY_BTN1, false, 2, 3, 1);
    physical_key(KC_GESTURE_SCREEN, false, 2, 4, 1);
    CHECK(stub_mouse_buttons == 0);

    reset_runtime();
    layer_on(1);
    physical_key(KC_WINDOW_SWITCH, true, 1, 2, 1);
    CHECK((stub_weak_modifiers & MOD_BIT_LALT) != 0);
    physical_key(KC_PROFILE, true, 7, 1, 1);
    CHECK((stub_weak_modifiers & MOD_BIT_LALT) == 0);
}

static void run_test(const char *name, void (*test)(void)) {
    current_test = name;
    current_failures = 0;
    test();
    printf("%s: %s\n", name, current_failures == 0 ? "PASS" : "FAIL");
}

int main(void) {
    run_test("drag_lock_click_keyup", test_drag_lock_click_releases_on_keyup);
    run_test("precision_idle_10s", test_precision_survives_idle_and_scales_small_motion);
    run_test("window_switch_weak_alt", test_window_switch_alt_survives_action_clear_and_releases_on_exit);
    run_test("mouse_k_hold", test_mouse_k_hold_enters_numbers_without_click_or_typing);
    run_test("gesture_session_reset", test_new_gesture_does_not_inherit_drag_distance);
    run_test("profile_switch_cleanup", test_profile_switch_releases_held_operations);
    if (total_failures != 0) {
        fprintf(stderr, "%u runtime integration assertion(s) failed\n", total_failures);
        return 1;
    }
    puts("6 runtime integration behaviors passed");
    return 0;
}
