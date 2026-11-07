/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H
#include "quantum.h"
#include "lib/keyball/keyball.h"
#include "transactions.h"
#include <string.h>

// OS detection is only available on newer QMK. Guard it so this keymap still
// builds on environments that don't ship the feature.
#if defined(OS_DETECTION_ENABLE) && defined(__has_include)
#  if __has_include("os_detection.h")
#    include "os_detection.h"
#    define HAS_OS_DETECTION 1
#  endif
#endif
#ifndef HAS_OS_DETECTION
#  define HAS_OS_DETECTION 0
#endif

// Private CoreX combo text is supplied only to local builds (see .gitignore).
#if defined(__has_include) && !defined(COREX_HOST_TEST)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif

#include "corex_port.h"
#include "corex_edit.h"
#include "corex_gesture.h"

static bool is_mac_profile(void) {
  return default_layer_state & ((layer_state_t)1 << L_MAC_BASE);
}

static void note_event(uint8_t event);
static void oled_state_sync(void);
static void oled_state_receive(uint8_t in_len, const void *in_data, uint8_t out_len, void *out_data);

static uint8_t mouse_layer(void) {
  return is_mac_profile() ? L_MAC_MOUSE : L_WIN_MOUSE;
}

// Mouse-layer K: tap = middle click, hold = numbers and scrolling.
static bool is_mouse_middle(uint16_t keycode) {
  return keycode == LT(L_MAC_NUM, KC_BTN3) || keycode == LT(L_WIN_NUM, KC_BTN3);
}

#define SCROLL_LAYERS_MASK (((layer_state_t)1 << L_WIN_NUM) | ((layer_state_t)1 << L_MAC_NUM))
#define MOUSE_LAYERS_MASK (((layer_state_t)1 << L_WIN_MOUSE) | ((layer_state_t)1 << L_MAC_MOUSE))
// Every momentary layer except the mouse layers takes priority over a mouse session.
#define MOUSE_BLOCKING_LAYERS_MASK (~(MOUSE_LAYERS_MASK | ((layer_state_t)1 << L_WIN_BASE) | ((layer_state_t)1 << L_MAC_BASE)))

typedef union {
  uint32_t raw;
  struct {
    // Preserve the EEPROM layout used by the previous state machine.
    int16_t reserved_click_movement;
    int16_t scroll_threshold;
  };
} user_config_t;

user_config_t user_config;

int16_t scroll_v_mouse_interval_counter; // 垂直スクロールの入力をカウントする。　Counting Vertical Scroll Inputs
int16_t scroll_h_mouse_interval_counter; // 水平スクロールの入力をカウントする。  Counts horizontal scrolling inputs.

bool gesture_key_held = false;
bool scroll_layer_active = false;
uint8_t held_mouse_buttons = 0;
uint8_t held_keyboard_keys = 0;
bool invert_scroll = false; // OS判定でスクロール方向を反転する
#if HAS_OS_DETECTION
uint8_t cached_os = 0;      // 一度だけ判定して保持
#endif

typedef enum {
  MOUSE_SESSION_NORMAL,
  MOUSE_SESSION_READY,
  MOUSE_SESSION_HELD,
  MOUSE_SESSION_MULTI_CLICK_GRACE,
  MOUSE_SESSION_WAIT_FOR_BALL_STOP,
} mouse_session_state_t;

enum {
  MOUSE_POSITION_BTN1,
  MOUSE_POSITION_BTN2,
  MOUSE_POSITION_BTN3,
  MOUSE_POSITION_BTN4,
  MOUSE_POSITION_BTN5,
  MOUSE_POSITION_GESTURE,
  MOUSE_POSITION_COUNT,
};


static mouse_session_state_t mouse_session_state = MOUSE_SESSION_NORMAL;
static uint32_t mouse_session_timer = 0;
static uint16_t mouse_session_movement = 0;
static uint32_t mouse_session_movement_timer = 0;
static uint16_t mouse_session_drag_distance = 0;
static uint8_t mouse_session_settle_reports = 0;
static bool mouse_session_ball_settled = false;
static bool mouse_session_used_scroll = false;
static bool pending_dual_role = false;
static keypos_t pending_dual_role_position;
static keypos_t held_mouse_positions[MOUSE_POSITION_COUNT];
static uint8_t held_mouse_position_mask = 0;
static keypos_t forced_release_positions[MOUSE_POSITION_COUNT];
static uint8_t forced_release_count = 0;

extern void register_mouse(uint8_t mouse_keycode, bool pressed);

static void mouse_layer_on(void) {
  if (!layer_state_is(mouse_layer())) {
    layer_on(mouse_layer());
  }
}

static void mouse_layer_off(void) {
  if (layer_state_is(mouse_layer())) {
    layer_off(mouse_layer());
  }
}

static bool is_minizone_scroll_active(void) {
  return scroll_layer_active;
}

int16_t my_abs(int16_t num);

static bool drag_locked, precision, window_switch_held;
static int8_t precision_x, precision_y;
static bool aml_enabled = true;
static uint8_t held_gesture_kind;
static bool profile_fixed;

static uint8_t gesture_kind(uint16_t kc) {
  switch (kc) {
    case KC_GESTURE_SCREEN: return 1;
    case KC_GESTURE_WINDOW: return 2;
    case KC_GESTURE_TAB: return 3;
    case KC_GESTURE_ZOOM: return 4;
    default: return 0;
  }
}

static void release_window_switch(void) {
  if (window_switch_held) {
    del_weak_mods(MOD_LALT);
    send_keyboard_report();
    window_switch_held = false;
  }
}

static void reset_scroll_counters(void) {
  scroll_v_mouse_interval_counter = 0;
  scroll_h_mouse_interval_counter = 0;
}

static bool key_positions_equal(keypos_t left, keypos_t right) {
  return left.row == right.row && left.col == right.col;
}

static void reset_mouse_session_movement(void) {
  mouse_session_movement = 0;
  mouse_session_movement_timer = timer_read32();
}

static void reset_mouse_session_settle(void) {
  mouse_session_settle_reports = 0;
  mouse_session_ball_settled = false;
}

static void remember_held_mouse_position(uint8_t index, keypos_t position) {
  held_mouse_positions[index] = position;
  held_mouse_position_mask |= (1U << index);
}

static void forget_held_mouse_position(uint8_t index) {
  held_mouse_position_mask &= ~(1U << index);
}

static void remember_forced_release(keypos_t position) {
  for (uint8_t i = 0; i < forced_release_count; i++) {
    if (key_positions_equal(forced_release_positions[i], position)) {
      return;
    }
  }
  if (forced_release_count < MOUSE_POSITION_COUNT) {
    forced_release_positions[forced_release_count++] = position;
  }
}

static bool consume_forced_release(keyrecord_t *record) {
  if (record->event.pressed) {
    return false;
  }
  for (uint8_t i = 0; i < forced_release_count; i++) {
    if (key_positions_equal(forced_release_positions[i], record->event.key)) {
      forced_release_positions[i] = forced_release_positions[--forced_release_count];
      return true;
    }
  }
  return false;
}

static void mouse_session_reset_tracking(void) {
  mouse_session_drag_distance = 0;
  mouse_session_used_scroll = false;
  reset_mouse_session_movement();
  reset_mouse_session_settle();
}

static void mouse_session_release_held_operations(void) {
  for (uint8_t i = 0; i < 5; i++) {
    if (held_mouse_buttons & (1U << i)) {
      register_mouse(KC_MS_BTN1 + i, false);
      if (held_mouse_position_mask & (1U << i)) {
        remember_forced_release(held_mouse_positions[i]);
      }
    }
  }
  held_mouse_buttons = 0;
  drag_locked = precision = false;
  precision_x = precision_y = 0;

  if (gesture_key_held) {
    if (held_mouse_position_mask & (1U << MOUSE_POSITION_GESTURE)) {
      remember_forced_release(held_mouse_positions[MOUSE_POSITION_GESTURE]);
    }
    gesture_key_held = false;
    held_gesture_kind = 0;
    corex_gesture_cancel();
  }
  held_mouse_position_mask = 0;
}

static void mouse_session_force_normal(bool update_layer) {
  mouse_session_release_held_operations();
  mouse_session_state = MOUSE_SESSION_NORMAL;
  mouse_session_reset_tracking();
  if (update_layer) {
    mouse_layer_off();
  }
}

static void mouse_session_enter_ready(void) {
  mouse_session_state = MOUSE_SESSION_READY;
  mouse_session_timer = timer_read32();
  reset_mouse_session_movement();
  mouse_layer_on();
}

static void mouse_session_enter_grace(void) {
  mouse_session_state = MOUSE_SESSION_MULTI_CLICK_GRACE;
  mouse_session_timer = timer_read32();
  reset_mouse_session_movement();
  reset_mouse_session_settle();
}

static void mouse_session_enter_wait(void) {
  mouse_session_state = MOUSE_SESSION_WAIT_FOR_BALL_STOP;
  reset_mouse_session_movement();
  reset_mouse_session_settle();
  mouse_layer_off();
}

static void mouse_session_finish_operation(void) {
  if (held_mouse_buttons != 0 || gesture_key_held) {
    return;
  }
  if (precision) {
    mouse_session_enter_ready();
    return;
  }
  if (mouse_session_used_scroll || mouse_session_drag_distance >= MOUSE_SESSION_DRAG_THRESHOLD) {
    mouse_session_enter_wait();
  } else {
    mouse_session_enter_grace();
  }
}

static bool mouse_session_meaningful_movement(uint16_t movement) {
  if (movement == 0) {
    return false;
  }
  if (timer_elapsed32(mouse_session_movement_timer) > MOUSE_SESSION_ACTIVATION_WINDOW) {
    mouse_session_movement = 0;
  }
  mouse_session_movement_timer = timer_read32();
  if (UINT16_MAX - mouse_session_movement < movement) {
    mouse_session_movement = UINT16_MAX;
  } else {
    mouse_session_movement += movement;
  }
  if (mouse_session_movement < MOUSE_SESSION_ACTIVATION_DISTANCE) {
    return false;
  }
  mouse_session_movement = 0;
  return true;
}

static void set_profile(bool mac) {
  mouse_session_force_normal(true);
  corex_edit_cancel();
  corex_gesture_cancel();
  release_window_switch();
  clear_keyboard();
  layer_clear();
  default_layer_set((layer_state_t)1 << (mac ? L_MAC_BASE : L_WIN_BASE));
  invert_scroll = !mac;  // Windows scrolls inverted, as with OS detection
}

void eeconfig_init_user(void) {
  user_config.raw = 0;
  user_config.scroll_threshold = 30;
  eeconfig_update_user(user_config.raw);
}

// Registered before the split link starts, so the first state sent is not dropped.
void keyboard_pre_init_user(void) {
  transaction_register_rpc(USER_OLED_SYNC, oled_state_receive);
}

void keyboard_post_init_user(void) {
  user_config.raw = eeconfig_read_user();
  bool config_changed = false;

  if (user_config.reserved_click_movement != 0) {
    user_config.reserved_click_movement = 0;
    config_changed = true;
  }
  if (user_config.scroll_threshold < 1 || user_config.scroll_threshold > 200) {
    user_config.scroll_threshold = 30;
    config_changed = true;
  }
  if (config_changed) {
    eeconfig_update_user(user_config.raw);
  }

  mouse_session_force_normal(false);

#if HAS_OS_DETECTION
  // OS自動判定 (少し待ってから実行する必要あり)
  wait_ms(400);
  cached_os = detected_host_os();
  set_profile(cached_os == OS_MACOS || cached_os == OS_IOS);
#endif
}

// 自前の絶対数を返す関数。 Functions that return absolute numbers.
int16_t my_abs(int16_t num)
{
  if (num < 0)
  {
    num = -num;
  }

  return num;
}

// Ending a session inside process_record_user must not switch layers yet:
// QMK looks up the pressed key's action after process_record_user returns.
// Turning the mouse layer off first would run the base-layer key instead.
static bool mouse_layer_exit_pending = false;

static void leave_mouse_session(void) {
  mouse_session_force_normal(false);
  mouse_layer_exit_pending = true;
}

// QMK skips post_process_record_user when process_record_user returns false,
// so keys that send their own output call this directly.
static void finish_mouse_layer_exit(void) {
  if (mouse_layer_exit_pending) {
    mouse_layer_exit_pending = false;
    if (mouse_session_state == MOUSE_SESSION_NORMAL) {
      mouse_layer_off();
    }
  }
}

void post_process_record_user(uint16_t keycode, keyrecord_t *record) {
  finish_mouse_layer_exit();
}

// Windows keeps the JIS layout, while the keymap is written with US symbols.
// Returns the JIS key that types the US symbol, or 0 when the key is the same.
static uint16_t jis_key(uint8_t kc, bool shift) {
  if (!shift) {
    switch (kc) {
      case KC_EQL:  return S(KC_MINS);
      case KC_LBRC: return KC_RBRC;
      case KC_RBRC: return KC_BSLS;
      case KC_BSLS: return KC_INT3;
      case KC_QUOT: return S(KC_7);
      case KC_GRV:  return S(KC_LBRC);
      default:      return 0;
    }
  }
  switch (kc) {
    case KC_2:    return KC_LBRC;
    case KC_6:    return KC_EQL;
    case KC_7:    return S(KC_6);
    case KC_8:    return S(KC_QUOT);
    case KC_9:    return S(KC_8);
    case KC_0:    return S(KC_9);
    case KC_MINS: return S(KC_INT1);
    case KC_EQL:  return S(KC_SCLN);
    case KC_LBRC: return S(KC_RBRC);
    case KC_RBRC: return S(KC_BSLS);
    case KC_BSLS: return S(KC_INT3);
    case KC_SCLN: return KC_QUOT;
    case KC_QUOT: return S(KC_2);
    case KC_GRV:  return S(KC_EQL);
    default:      return 0;
  }
}

// Types a US symbol on the Windows profile as its JIS key. Shortcuts with
// Ctrl, Alt or Win pass through untouched. Returns true when it typed the key.
static bool send_as_jis(uint16_t keycode, keyrecord_t *record) {
  if (is_mac_profile() || !record->event.pressed) {
    return false;
  }
  if (IS_QK_MOD_TAP(keycode)) {
    keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
  } else if (IS_QK_LAYER_TAP(keycode)) {
    keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
  }
  uint8_t mods = get_mods();
  if ((mods & ~MOD_MASK_SHIFT) || keycode > QK_MODS_MAX || (QK_MODS_GET_MODS(keycode) & ~(MOD_LSFT | MOD_RSFT))) {
    return false;
  }
  uint16_t target = jis_key(QK_MODS_GET_BASIC_KEYCODE(keycode), (mods & MOD_MASK_SHIFT) || QK_MODS_GET_MODS(keycode));
  if (!target) {
    return false;
  }
  del_mods(MOD_MASK_SHIFT);
  tap_code16(target);
  set_mods(mods);
  send_keyboard_report();
  return true;
}

/* Emit complete chords without borrowing held thumb modifiers. */
void corex_edit_emit(uint16_t chord) {
  uint8_t mods = get_mods(), weak = get_weak_mods();
  clear_mods();
  clear_weak_mods();
  send_keyboard_report();
  tap_code16(chord);
  set_mods(mods);
  set_weak_mods(weak);
  send_keyboard_report();
}

void corex_gesture_emit(uint16_t chord) {
  note_event(held_gesture_kind);
  corex_edit_emit(chord);
}

static void type_us_key(uint16_t kc) {
  uint16_t target = is_mac_profile() ? 0 : jis_key(kc & 0xFF, (kc >> 8) & MOD_LSFT);
  corex_edit_emit(target ? target : kc);
}

#if defined(COREX_PRIVATE_1) || defined(COREX_PRIVATE_2)
static void type_private_P(const uint16_t *keys, uint8_t length) {
  for (uint8_t i = 0; i < length; i++) type_us_key(pgm_read_word(&keys[i]));
}
#endif

static void run_macro(uint16_t kc) {
  static const uint16_t PROGMEM pairs[][2] = {
    {KC_LBRC, KC_RBRC}, {S(KC_9), S(KC_0)}, {S(KC_LBRC), S(KC_RBRC)},
    {S(KC_QUOT), S(KC_QUOT)}, {KC_GRV, KC_GRV}
  };
  if (kc >= KC_PAIR_BRACKETS && kc <= KC_PAIR_TICKS) {
    uint8_t pair = kc - KC_PAIR_BRACKETS;
    type_us_key(pgm_read_word(&pairs[pair][0]));
    type_us_key(pgm_read_word(&pairs[pair][1]));
    corex_edit_emit(KC_LEFT);
  } else if (kc == KC_CODE_BLOCK) {
    for (uint8_t i = 0; i < 3; i++) type_us_key(KC_GRV);
    corex_edit_emit(KC_ENT); corex_edit_emit(KC_ENT);
    for (uint8_t i = 0; i < 3; i++) type_us_key(KC_GRV);
    corex_edit_emit(KC_UP);
  } else if (kc == KC_DOUBLE_ZERO) {
    corex_edit_emit(KC_0); corex_edit_emit(KC_0);
  }
#ifdef COREX_PRIVATE_1
  else if (kc == KC_PRIVATE_1) {
    static const uint16_t PROGMEM keys[] = COREX_PRIVATE_1;
    type_private_P(keys, sizeof(keys) / sizeof(keys[0]));
  }
#endif
#ifdef COREX_PRIVATE_2
  else if (kc == KC_PRIVATE_2) {
    static const uint16_t PROGMEM keys[] = COREX_PRIVATE_2;
    type_private_P(keys, sizeof(keys) / sizeof(keys[0]));
  }
#endif
}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
  // QMK clears weak modifiers before every physical key press.
  if (window_switch_held) add_weak_mods(MOD_LALT);
  if (IS_QK_LAYER_TAP(keycode)) {
    // Both the tap and hold meanings leave the mouse session, so exit before
    // QMK resolves a following key against the mouse layer. The mouse-layer K
    // is a mouse operation when tapped, so it keeps the session.
    if (!is_mouse_middle(keycode) && record->event.pressed && mouse_session_state != MOUSE_SESSION_NORMAL) {
      mouse_session_force_normal(true);
    }
  } else if (IS_QK_MOD_TAP(keycode)) {
    if (record->event.pressed) {
      pending_dual_role = true;
      pending_dual_role_position = record->event.key;
    } else if (pending_dual_role && key_positions_equal(pending_dual_role_position, record->event.key)) {
      pending_dual_role = false;
    }
  }
  return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{
  if (consume_forced_release(record)) {
    return false;
  }
  uint8_t action_id = keycode >= KEYBALL_SAFE_RANGE && keycode < KEYBALL_SAFE_RANGE + 64
                      ? keycode - KEYBALL_SAFE_RANGE : 0;
  if (corex_edit_process(action_id, record->event.pressed, is_mac_profile(),
                         (get_mods() | get_weak_mods()) & MOD_MASK_SHIFT, timer_read())) {
    if (record->event.pressed) mouse_session_force_normal(true);
    return false;
  }

  bool is_mod_tap = IS_QK_MOD_TAP(keycode);
  bool is_layer_tap = IS_QK_LAYER_TAP(keycode);
  if ((is_mod_tap || is_layer_tap) && pending_dual_role &&
      key_positions_equal(pending_dual_role_position, record->event.key)) {
    pending_dual_role = false;
  }

  if (is_mod_tap && record->tap.count == 0) {
    if (record->event.pressed) {
      if (mouse_session_state == MOUSE_SESSION_READY ||
          mouse_session_state == MOUSE_SESSION_MULTI_CLICK_GRACE) {
        mouse_session_timer = timer_read32();
      }
    }
    return true;
  }

  if (is_layer_tap && record->tap.count == 0) {
    if (record->event.pressed && mouse_session_state != MOUSE_SESSION_NORMAL) {
      leave_mouse_session();
    }
    return true;
  }

  if (IS_MODIFIER_KEYCODE(keycode)) {
    if (record->event.pressed &&
        (mouse_session_state == MOUSE_SESSION_READY ||
         mouse_session_state == MOUSE_SESSION_MULTI_CLICK_GRACE)) {
      mouse_session_timer = timer_read32();
    }
    return true;
  }

  if (is_mouse_middle(keycode)) {
    keycode = KC_MY_BTN3;  // tap: middle click as a session-aware button
  }

  switch (keycode)
  {
  case KC_MY_BTN1:
  case KC_MY_BTN2:
  case KC_MY_BTN3:
  case KC_MY_BTN4:
  case KC_MY_BTN5:
  {
    uint8_t button_index = keycode - KC_MY_BTN1;
    uint8_t button_mask = 1 << button_index;

    if (button_index == 0 && record->event.pressed) drag_locked = false;
    if (!(button_index == 0 && drag_locked && !record->event.pressed))
      register_mouse(KC_MS_BTN1 + button_index, record->event.pressed);
    if (record->event.pressed) {
      if (mouse_session_state != MOUSE_SESSION_HELD) {
        mouse_session_drag_distance = 0;
        mouse_session_used_scroll = false;
      }
      held_mouse_buttons |= button_mask;
      remember_held_mouse_position(button_index, record->event.key);
      mouse_session_state = MOUSE_SESSION_HELD;
      mouse_layer_on();
    } else {
      if (!(button_index == 0 && drag_locked)) held_mouse_buttons &= ~button_mask;
      forget_held_mouse_position(button_index);
      if (mouse_session_state == MOUSE_SESSION_HELD) {
        mouse_session_finish_operation();
      }
    }
    return false;
  }

  case KC_GESTURE_SCREEN:
  case KC_GESTURE_WINDOW:
  case KC_GESTURE_TAB:
  case KC_GESTURE_ZOOM: {
    uint8_t kind = gesture_kind(keycode);
    if (record->event.pressed && held_gesture_kind && kind != held_gesture_kind) return false;
    if (!record->event.pressed && kind != held_gesture_kind) return false;
    corex_gesture_key(kind, record->event.pressed, is_mac_profile(), timer_read());
    gesture_key_held = record->event.pressed;
    if (record->event.pressed) {
      if (mouse_session_state != MOUSE_SESSION_HELD) {
        mouse_session_drag_distance = 0;
        mouse_session_used_scroll = false;
      }
      held_gesture_kind = kind;
      remember_held_mouse_position(MOUSE_POSITION_GESTURE, record->event.key);
      mouse_session_state = MOUSE_SESSION_HELD;
      mouse_layer_on();
    } else {
      held_gesture_kind = 0;
      forget_held_mouse_position(MOUSE_POSITION_GESTURE);
      mouse_session_finish_operation();
    }
    return false;
  }
  case KC_DRAG_LOCK:
    if (record->event.pressed) {
      drag_locked = !drag_locked;
      bool down = drag_locked || (held_mouse_position_mask & 1);
      register_mouse(KC_MS_BTN1, down);
      if (down) { held_mouse_buttons |= 1; mouse_session_state = MOUSE_SESSION_HELD; }
      else { held_mouse_buttons &= ~1; mouse_session_finish_operation(); }
    }
    return false;
  case KC_PRECISION:
    if (record->event.pressed) {
      precision = !precision;
      precision_x = precision_y = 0;
      if (precision) {
        if (!held_mouse_buttons && !gesture_key_held) mouse_session_enter_ready();
      } else mouse_session_finish_operation();
    }
    return false;
  case KC_DOUBLE_CLICK:
    if (record->event.pressed) {
      for (uint8_t i = 0; i < 2; i++) {
        register_mouse(KC_MS_BTN1, true); wait_ms(40);
        register_mouse(KC_MS_BTN1, false); wait_ms(40);
      }
      mouse_session_enter_grace();
    }
    return false;
  case KC_WINDOW_SWITCH:
    if (record->event.pressed) {
      add_weak_mods(MOD_LALT);
      window_switch_held = true;
      tap_code(KC_TAB);
    }
    return false;
  case KC_CURSOR_SLOWER:
  case KC_CURSOR_FASTER:
    if (record->event.pressed) {
      uint8_t cpi = keyball_get_cpi();
      if (keycode == KC_CURSOR_FASTER && cpi < 120) cpi++;
      if (keycode == KC_CURSOR_SLOWER && cpi > 1) cpi--;
      keyball_set_cpi(cpi);
    }
    return false;
  case KC_SCROLL_LESS:
  case KC_SCROLL_MORE:
    if (record->event.pressed) {
      int16_t speed = user_config.scroll_threshold + (keycode == KC_SCROLL_LESS ? 5 : -5);
      user_config.scroll_threshold = speed < 1 ? 1 : speed > 200 ? 200 : speed;
      eeconfig_update_user(user_config.raw);
    }
    return false;
  case KC_GESTURE_DULLER:
  case KC_GESTURE_SHARPER:
    if (record->event.pressed) corex_gesture_sensitivity(keycode == KC_GESTURE_SHARPER ? 1 : -1);
    return false;
  case KC_AML_TOGGLE:
    if (record->event.pressed) { aml_enabled = !aml_enabled; mouse_session_force_normal(true); }
    return false;
  case KC_SCROLL_INVERT:
    if (record->event.pressed) invert_scroll = !invert_scroll;
    return false;
  case KC_PROFILE:
    if (record->event.pressed) { set_profile(!is_mac_profile()); profile_fixed = true; }
    return false;
  case KC_SHOW_STATUS:
    return false; /* Both OLEDs continuously display status. */
  case KC_PAIR_BRACKETS ... KC_PRIVATE_2:
    if (record->event.pressed) run_macro(keycode);
    return false;

  default:
    break;
  }

  if (record->event.pressed) {
    if (held_keyboard_keys < UINT8_MAX) {
      held_keyboard_keys++;
    }
    if (mouse_session_state != MOUSE_SESSION_NORMAL) {
      leave_mouse_session();
    }
  } else if (held_keyboard_keys > 0) {
    held_keyboard_keys--;
  }

  reset_mouse_session_movement();

  if (send_as_jis(keycode, record)) {
    finish_mouse_layer_exit();
    return false;
  }
  return true;
}

static void mouse_session_process_pointing(uint16_t movement) {
  switch (mouse_session_state) {
    case MOUSE_SESSION_NORMAL:
      if (held_keyboard_keys > 0 || (layer_state & MOUSE_BLOCKING_LAYERS_MASK)) {
        reset_mouse_session_movement();
      } else if (mouse_session_meaningful_movement(movement)) {
        mouse_session_enter_ready();
      }
      break;

    case MOUSE_SESSION_READY:
      if (mouse_session_meaningful_movement(movement)) {
        mouse_session_timer = timer_read32();
      }
      break;

    case MOUSE_SESSION_HELD:
      if (movement > 0) {
        if (UINT16_MAX - mouse_session_drag_distance < movement) {
          mouse_session_drag_distance = UINT16_MAX;
        } else {
          mouse_session_drag_distance += movement;
        }
        if (gesture_key_held) {
          mouse_session_used_scroll = true;
        }
      }
      break;

    case MOUSE_SESSION_MULTI_CLICK_GRACE:
      if (movement == 0) {
        if (!mouse_session_ball_settled &&
            ++mouse_session_settle_reports >= MOUSE_SESSION_BALL_STOP_REPORTS) {
          mouse_session_ball_settled = true;
          reset_mouse_session_movement();
        }
      } else if (!mouse_session_ball_settled) {
        mouse_session_settle_reports = 0;
        reset_mouse_session_movement();
      } else if (mouse_session_meaningful_movement(movement)) {
        mouse_session_enter_ready();
      }
      break;

    case MOUSE_SESSION_WAIT_FOR_BALL_STOP:
      if (movement == 0) {
        if (++mouse_session_settle_reports >= MOUSE_SESSION_BALL_STOP_REPORTS) {
          mouse_session_force_normal(false);
        }
      } else {
        mouse_session_settle_reports = 0;
      }
      break;
  }
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report)
{
  int16_t current_x = mouse_report.x;
  int16_t current_y = mouse_report.y;
  int16_t current_h = mouse_report.h;
  int16_t current_v = mouse_report.v;
  uint16_t raw_movement = my_abs(current_x) + my_abs(current_y);

  if (aml_enabled) mouse_session_process_pointing(raw_movement);

  if (gesture_key_held) {
    corex_gesture_motion(current_x, current_y, is_mac_profile(), timer_read());
    current_x = 0;
    current_y = 0;
  } else if (is_minizone_scroll_active() && (current_x != 0 || current_y != 0)) {
      int8_t rep_v = 0;
      int8_t rep_h = 0;
      int16_t sv_th = user_config.scroll_threshold < 1 ? 1 : user_config.scroll_threshold; // vertical threshold
      int16_t sh_th = sv_th * 2; // horizontal threshold is 2x vertical

      // 垂直スクロールの方の感度を高める。 Increase sensitivity toward vertical scrolling.
      if (my_abs(current_y) * 2 > my_abs(current_x)) {

        scroll_v_mouse_interval_counter += current_y;
        while (my_abs(scroll_v_mouse_interval_counter) >= sv_th) {
          if (scroll_v_mouse_interval_counter < 0) {
            scroll_v_mouse_interval_counter += sv_th;
            rep_v++;
          } else {
            scroll_v_mouse_interval_counter -= sv_th;
            rep_v--;
          }
        }
      } else {
        scroll_h_mouse_interval_counter += current_x;

        while (my_abs(scroll_h_mouse_interval_counter) >= sh_th) {
          if (scroll_h_mouse_interval_counter < 0) {
            scroll_h_mouse_interval_counter += sh_th;
            rep_h++;
          } else {
            scroll_h_mouse_interval_counter -= sh_th;
            rep_h--;
          }
        }
      }

      current_h = rep_h;
      current_v = -rep_v;
      if (invert_scroll) {
        current_h = -current_h;
        current_v = -current_v;
      }
      current_x = 0;
      current_y = 0;
  }

  if (precision && !scroll_layer_active && !gesture_key_held) {
    current_x += precision_x; current_y += precision_y;
    precision_x = current_x % 4; precision_y = current_y % 4;
    current_x /= 4; current_y /= 4;
  }
  mouse_report.x = current_x;
  mouse_report.y = current_y;
  mouse_report.h = current_h;
  mouse_report.v = current_v;
  keyball.last_mouse = mouse_report; // propagate post-processed values to OLED

  return mouse_report;
}

void housekeeping_task_user(void) {
  corex_edit_tick(is_mac_profile(), timer_read());
  oled_state_sync();
  if (pending_dual_role || precision) {
    return;
  }

  if (mouse_session_state == MOUSE_SESSION_READY &&
      timer_elapsed32(mouse_session_timer) >= MOUSE_SESSION_READY_TIMEOUT) {
    mouse_session_force_normal(true);
  } else if (mouse_session_state == MOUSE_SESSION_MULTI_CLICK_GRACE &&
             timer_elapsed32(mouse_session_timer) >= MOUSE_SESSION_MULTI_CLICK_TERM) {
    if (mouse_session_ball_settled) {
      mouse_session_force_normal(true);
    } else {
      mouse_session_enter_wait();
    }
  }
}

////////////////////////////////////
///
/// 自動マウスレイヤーの実装 ここまで
///
////////////////////////////////////

#include "corex_keymaps.inc"

bool combo_should_trigger(uint16_t combo_index, combo_t *combo, uint16_t keycode, keyrecord_t *record) {
  return get_highest_layer(layer_state | default_layer_state) % PROFILE_LAYERS == L_WIN_BASE;
}

bool get_combo_must_hold(uint16_t combo_index, combo_t *combo) {
  return combo_index == COMBO_SETTINGS;
}

void process_combo_event(uint16_t combo_index, bool pressed) {
  if (combo_index == COMBO_SETTINGS) {
    uint8_t settings = is_mac_profile() ? L_MAC_SETTINGS : L_WIN_SETTINGS;
    if (pressed) { mouse_session_force_normal(true); layer_on(settings); }
    else layer_off(settings);
  } else if (pressed) {
    mouse_session_force_normal(true);
    run_macro(combo_index == COMBO_PRIVATE_1 ? KC_PRIVATE_1 : KC_PRIVATE_2);
  }
}

layer_state_t layer_state_set_user(layer_state_t ly_state)
{
  if (!(ly_state & (((layer_state_t)1 << L_WIN_EDIT) | ((layer_state_t)1 << L_MAC_EDIT))))
    release_window_switch();
  bool next_scroll_layer_active = (ly_state & SCROLL_LAYERS_MASK) != 0;
  bool blocking_layer_active = (ly_state & MOUSE_BLOCKING_LAYERS_MASK) != 0;
  bool mouse_layer_active = (ly_state & MOUSE_LAYERS_MASK) != 0;
  bool session_requires_mouse_layer =
      mouse_session_state == MOUSE_SESSION_READY ||
      mouse_session_state == MOUSE_SESSION_HELD ||
      mouse_session_state == MOUSE_SESSION_MULTI_CLICK_GRACE;

  // Normal layers always win over the temporary mouse layer.
  if (blocking_layer_active) {
    if (mouse_session_state != MOUSE_SESSION_NORMAL) {
      mouse_session_force_normal(false);
    }
    ly_state &= ~MOUSE_LAYERS_MASK;
  } else if (!mouse_layer_active && session_requires_mouse_layer) {
    // Also recover safely if a mouse layer is disabled outside the session module.
    mouse_session_force_normal(false);
  }

  if (scroll_layer_active != next_scroll_layer_active) {
    scroll_layer_active = next_scroll_layer_active;
    reset_scroll_counters();
  }
  keyball_set_scroll_mode(false);

  return ly_state;
}

////////////////////////////////////
///
/// OLED: the left-hand screen shows the current state, the right-hand screen
/// what just happened. The USB side sends what both screens need to the other
/// side, so each hand shows the same thing whichever side is plugged in.
///
////////////////////////////////////

#define EVENT_SHOW_MS 1500

enum oled_flag {
  FLAG_MAC = 1 << 0,
  FLAG_FIXED = 1 << 1,
  FLAG_SCROLL = 1 << 2,
  FLAG_CLICK = 1 << 3,
  FLAG_GESTURE = 1 << 4,
};

typedef struct {
  uint8_t layer;   // 0-5, the same on both profiles
  uint8_t mods;
  uint8_t flags;
  uint8_t event;
  uint8_t seq;     // changes with every event, so the other side restarts its timer
  uint8_t cpi;
  uint8_t scroll;
} oled_state_t;

static oled_state_t shown;        // what this side's screen shows
static uint16_t event_timer = 0;

static void note_event(uint8_t event) {
  if (!event) return;
  shown.event = event;
  shown.seq++;
  event_timer = timer_read();
}

static bool event_active(void) {
  return shown.event != 0 && timer_elapsed(event_timer) < EVENT_SHOW_MS;
}

// USB side: refresh the state from the keyboard and send it when it changes.
static void oled_state_sync(void) {
  if (!is_keyboard_master()) {
    return;
  }
  shown.layer = get_highest_layer(layer_state | default_layer_state) % PROFILE_LAYERS;
  shown.mods = get_mods();
  shown.flags = (is_mac_profile() ? FLAG_MAC : 0) | (profile_fixed ? FLAG_FIXED : 0) |
                (gesture_key_held ? FLAG_GESTURE : 0) | (is_minizone_scroll_active() ? FLAG_SCROLL : 0) |
                (held_mouse_buttons != 0 ? FLAG_CLICK : 0);
  shown.cpi = keyball_get_cpi();
  shown.scroll = user_config.scroll_threshold;
  // Clear an expired event here, on the USB side only; otherwise the 16-bit
  // timer wraps after 65 s and the old event shows again.
  if (shown.event != 0 && timer_elapsed(event_timer) >= EVENT_SHOW_MS) {
    shown.event = 0;
  }

  // A failed send leaves `sent` unchanged, so it is retried on the next pass.
  static oled_state_t sent;
  if (memcmp(&sent, &shown, sizeof(shown)) != 0 && transaction_rpc_send(USER_OLED_SYNC, sizeof(shown), &shown)) {
    sent = shown;
  }
}

static void oled_state_receive(uint8_t in_len, const void *in_data, uint8_t out_len, void *out_data) {
  if (in_len != sizeof(oled_state_t)) {
    return;
  }
  uint8_t seq = shown.seq;
  memcpy(&shown, in_data, sizeof(shown));
  if (shown.seq != seq) {
    event_timer = timer_read();
  }
}

#ifdef OLED_ENABLE

extern const unsigned char font[] PROGMEM;

// The lit area sits off-centre on the glass, so three rows land in the middle
// of the window only when the empty fourth row is on the narrow-margin side.
// The right hand turns 180 degrees and uses rows 0-2 (the Keyball logo's
// trick); the left hand keeps its text facing the user and uses rows 1-3.
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
  return is_keyboard_left() ? OLED_ROTATION_0 : OLED_ROTATION_180;
}

static uint8_t top_row(void) {
  return is_keyboard_left() ? 1 : 0;
}

static const char *layer_name(uint8_t layer) {
  switch (layer) {
    case L_WIN_EDIT: return PSTR("EDIT");
    case L_WIN_MOUSE: return PSTR("MOUSE");
    case L_WIN_SYMBOL: return PSTR("SYMBOL");
    case L_WIN_SETTINGS: return PSTR("SETTINGS");
    case L_WIN_NUM: return PSTR("NUM");
    default: return PSTR("BASE");
  }
}

// Spread 4 bits over 8, each pixel twice as tall.
static uint8_t double_bits(uint8_t nibble) {
  uint8_t out = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (nibble & (1 << i)) {
      out |= 3 << (i * 2);
    }
  }
  return out;
}

static uint16_t glyph_offset(char c) {
  uint8_t glyph = c == ' ' ? 0 : c >= 'A' ? c - 'A' + 11 : c - '0' + 1;
  return glyph * OLED_FONT_WIDTH;
}

// One character at twice the size over rows 0 and 1; returns the next x.
static uint8_t write_big_glyph(char c, uint8_t x) {
  for (uint8_t i = 0; i < OLED_FONT_WIDTH; i++) {
    uint8_t col = pgm_read_byte(&font[glyph_offset(c) + i]);
    for (uint8_t twice = 0; twice < 2; twice++, x++) {
      uint16_t at = top_row() * OLED_DISPLAY_WIDTH + x;
      oled_write_raw_byte(double_bits(col & 0x0F), at);
      oled_write_raw_byte(double_bits(col >> 4), OLED_DISPLAY_WIDTH + at);
    }
  }
  return x;
}

static void write_big_P(const char *text) {
  uint8_t x = 0;
  for (char c; (c = pgm_read_byte(text)) && x + 2 * OLED_FONT_WIDTH <= OLED_DISPLAY_WIDTH; text++) {
    x = write_big_glyph(c, x);
  }
}

static void small_char(char c, uint8_t column, uint8_t row, bool invert) {
  for (uint8_t i = 0; i < OLED_FONT_WIDTH; i++) {
    uint8_t pixel = pgm_read_byte(&font[glyph_offset(c) + i]);
    oled_write_raw_byte(invert ? ~pixel : pixel, row * OLED_DISPLAY_WIDTH + column * OLED_FONT_WIDTH + i);
  }
}

static void small_P(const char *text, uint8_t column, uint8_t row, bool invert) {
  char c;
  while ((c = pgm_read_byte(text++))) small_char(c, column++, row, invert);
}

static void small_number(uint8_t value, uint8_t column, uint8_t row) {
  small_char('0' + value / 100, column, row, false);
  small_char('0' + value / 10 % 10, column + 1, row, false);
  small_char('0' + value % 10, column + 2, row, false);
}

static void render_state(void) {
  bool mac = shown.flags & FLAG_MAC;
  write_big_P(layer_name(shown.layer));
  small_P(mac ? PSTR("MAC") : PSTR("WIN"), 17, 1, false);
  if (shown.flags & FLAG_FIXED) small_P(PSTR("FIX"), 17, 2, true);
  small_P(PSTR("SFT"), 0, 3, shown.mods & MOD_MASK_SHIFT);
  small_P(PSTR("CTL"), 5, 3, shown.mods & MOD_MASK_CTRL);
  small_P(mac ? PSTR("OPT") : PSTR("ALT"), 10, 3, shown.mods & MOD_MASK_ALT);
  small_P(mac ? PSTR("CMD") : PSTR("WIN"), 15, 3, shown.mods & MOD_MASK_GUI);
}

static const char *event_name(uint8_t event) {
  switch (event) {
    case 1: return PSTR("SCREEN");
    case 2: return PSTR("WINDOW");
    case 3: return PSTR("TAB");
    default: return PSTR("ZOOM");
  }
}

static void render_activity(void) {
  const char *label = event_active() ? event_name(shown.event) :
      shown.flags & FLAG_GESTURE ? PSTR("GESTURE") :
      shown.flags & FLAG_SCROLL ? PSTR("SCROLL") :
      shown.flags & FLAG_CLICK ? PSTR("CLICK") : PSTR("READY");
  write_big_P(label);
  small_P(PSTR("CPI"), 0, 2, false);
  small_number(shown.cpi, 3, 2);
  small_P(PSTR("00 SC"), 6, 2, false);
  small_number(shown.scroll, 11, 2);
}

bool oled_task_user(void) {
  // Redraw only when something on this side's screen changes.
  static oled_state_t drawn;
  static bool drawn_active = false;
  static bool drawn_once = false;
  bool active = event_active();
  if (drawn_once && active == drawn_active && memcmp(&drawn, &shown, sizeof(shown)) == 0) {
    return false;
  }
  drawn = shown;
  drawn_active = active;
  drawn_once = true;
  oled_clear();
  if (is_keyboard_left()) {
    render_state();
  } else {
    render_activity();
  }
  return false;
}
#endif
