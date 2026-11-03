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

// Text for the Windows macro keys lives outside Git (see .gitignore).
#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif

// コード表
// ## 特殊キーコード

// | キーコード | Remap上での表記 | 値       | 説明                                                              |
// |:-----------|:----------------|:---------|:------------------------------------------------------------------|
// | `KBC_RST`  | `Kb 0`          | `0x7e00` | Keyball設定[^2]のリセット                                         |
// | `KBC_SAVE` | `Kb 1`          | `0x7e01` | 現在のKeyball設定[^2]をEEPROMに保存します                         |
// | `CPI_I100` | `Kb 2`          | `0x7e02` | CPIを100増加させます(最大:12000)                                  |
// | `CPI_D100` | `Kb 3`          | `0x7e03` | CPIを100減少させます(最小:100)                                    |
// | `CPI_I1K`  | `Kb 4`          | `0x7e04` | CPIを1000増加させます(最大:12000)                                 |
// | `CPI_D1K`  | `Kb 5`          | `0x7e05` | CPIを1000減少させます(最小:100)                                   |
// | `SCRL_TO`  | `Kb 6`          | `0x7e06` | タップごとにスクロールモードのON/OFFを切り替えます                |
// | `SCRL_MO`  | `Kb 7`          | `0x7e07` | キーを押している間、スクロールモードになります                    |
// | `SCRL_DVI` | `Kb 8`          | `0x7e08` | スクロール除数を１つ上げます(max D7 = 1/128)←最もスクロール遅い   |
// | `SCRL_DVD` | `Kb 9`          | `0x7e09` | スクロール除数を１つ下げます(min D0 = 1/1)←最もスクロール速い     |
// | `AML_TO`   | `Kb 10`         | `0x7e0a` | 自動マウスレイヤーをトグルします。                                |
// | `AML_I50`  | `Kb 11`         | `0x7e0b` | 自動マウスレイヤーのタイムアウトを50msec増やします (max 1000ms)   |
// | `AML_D50`  | `Kb 12`         | `0x7e0c` | 自動マウスレイヤーのタイムアウトを50msec減らします (min 100ms)    |
// | `SSNP_VRT` | `Kb 13`         | `0x7e0d` | スクロールスナップモードを垂直にする                              |
// | `SSNP_HOR` | `Kb 14`         | `0x7e0e` | スクロールスナップモードを水平にする                              |
// | `SSNP_FRE` | `Kb 15`         | `0x7e0f` | スクロールスナップモードを無効にする(自由スクロール)              |

////////////////////////////////////
///
/// 自動マウスレイヤーの実装 ここから
/// 参考にさせていただいたページ
/// https://zenn.dev/takashicompany/articles/69b87160cda4b9
///
////////////////////////////////////

enum custom_keycodes {
  // corex_keymaps.inc refers to these by value (User 0-11), so keep them stable.
  KC_MY_BTN1            = KEYBALL_SAFE_RANGE + 0,  // User0
  KC_MY_BTN2            = KEYBALL_SAFE_RANGE + 1,  // User1
  KC_MY_BTN3            = KEYBALL_SAFE_RANGE + 2,  // User2
  KC_MY_BTN4            = KEYBALL_SAFE_RANGE + 3,  // User3
  KC_MY_BTN5            = KEYBALL_SAFE_RANGE + 4,  // User4
  KC_MY_SCR             = KEYBALL_SAFE_RANGE + 5,  // User5
  KC_TO_CLICKABLE_INC   = KEYBALL_SAFE_RANGE + 6,  // User6 (reserved)
  KC_TO_CLICKABLE_DEC   = KEYBALL_SAFE_RANGE + 7,  // User7 (reserved)
  KC_SCR_SPD_INC        = KEYBALL_SAFE_RANGE + 8,  // User8 スクロール閾値を下げて速度アップ
  KC_SCR_SPD_DEC        = KEYBALL_SAFE_RANGE + 9,  // User9 スクロール閾値を上げて速度ダウン
  KC_SCR_RST            = KEYBALL_SAFE_RANGE + 10, // User10 スクロール閾値を初期値に戻す
  KC_URL_COPY           = KEYBALL_SAFE_RANGE + 11  // Windows: アドレス欄の URL をコピー
};

// Both profiles carry the CoreX layers: Windows 0-4, macOS 5-9.
// The connected OS picks the default layer at boot; Shift + \ (held) switches by hand.
enum layers {
  L_WIN_BASE = 0,
  L_WIN_EDIT,
  L_WIN_MOUSE,
  L_WIN_WINDOW,
  L_WIN_NUM,
  L_MAC_BASE,
  L_MAC_EDIT,
  L_MAC_MOUSE,
  L_MAC_WINDOW,
  L_MAC_NUM,
};
#define PROFILE_LAYERS (L_MAC_BASE - L_WIN_BASE)

static bool is_mac_profile(void) {
  return default_layer_state & ((layer_state_t)1 << L_MAC_BASE);
}

static uint8_t mouse_layer(void) {
  return is_mac_profile() ? L_MAC_MOUSE : L_WIN_MOUSE;
}

// Mouse-layer K: tap = middle click, hold = window layer (ball scrolls there).
static bool is_mouse_middle(uint16_t keycode) {
  return keycode == LT(L_MAC_WINDOW, KC_BTN3) || keycode == LT(L_WIN_WINDOW, KC_BTN3);
}

static bool is_mouse_layer(uint8_t layer) {
  return layer == L_MAC_MOUSE || layer == L_WIN_MOUSE;
}

#define SCROLL_LAYERS_MASK (((layer_state_t)1 << L_WIN_WINDOW) | ((layer_state_t)1 << L_MAC_WINDOW))
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

bool scroll_key_held = false;
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
  MOUSE_POSITION_SCROLL,
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
// True while the mouse layer is held by hand (CoreX: かな長押し). The session must not drop it.
static bool mouse_layer_held = false;
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
  if (!mouse_layer_held && layer_state_is(mouse_layer())) {
    layer_off(mouse_layer());
  }
}

static bool is_minizone_scroll_active(void) {
  return scroll_key_held || scroll_layer_active;
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

  if (scroll_key_held) {
    if (held_mouse_position_mask & (1U << MOUSE_POSITION_SCROLL)) {
      remember_forced_release(held_mouse_positions[MOUSE_POSITION_SCROLL]);
    }
    scroll_key_held = false;
    reset_scroll_counters();
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
  if (held_mouse_buttons != 0 || scroll_key_held) {
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
  default_layer_set((layer_state_t)1 << (mac ? L_MAC_BASE : L_WIN_BASE));
  invert_scroll = !mac;  // Windows scrolls inverted, as with OS detection
}

void eeconfig_init_user(void) {
  user_config.raw = 0;
  user_config.scroll_threshold = 30;
  eeconfig_update_user(user_config.raw);
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

static void tap_key_sequence(const uint16_t *keys, uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    tap_code16(pgm_read_word(&keys[i]));
  }
}

bool pre_process_record_user(uint16_t keycode, keyrecord_t *record) {
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
    if (is_mouse_layer(QK_LAYER_TAP_GET_LAYER(keycode))) {
      // Holding かな opens the mouse layer by hand, like CoreX.
      mouse_layer_held = record->event.pressed;
      return true;
    }
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
      held_mouse_buttons &= ~button_mask;
      forget_held_mouse_position(button_index);
      if (mouse_session_state == MOUSE_SESSION_HELD) {
        mouse_session_finish_operation();
      }
    }
    return false;
  }

  case KC_MY_SCR:
    if (scroll_key_held != record->event.pressed) {
      scroll_key_held = record->event.pressed;
      reset_scroll_counters();
      if (record->event.pressed) {
        if (mouse_session_state != MOUSE_SESSION_HELD) {
          mouse_session_drag_distance = 0;
          mouse_session_used_scroll = false;
        }
        remember_held_mouse_position(MOUSE_POSITION_SCROLL, record->event.key);
        mouse_session_state = MOUSE_SESSION_HELD;
        mouse_layer_on();
      } else {
        forget_held_mouse_position(MOUSE_POSITION_SCROLL);
        if (mouse_session_state == MOUSE_SESSION_HELD) {
          mouse_session_finish_operation();
        }
      }
    }
    return false;

#ifdef WIN_MACRO_11_KEYS
  case QK_MACRO_11:
    if (record->event.pressed) {
      static const uint16_t PROGMEM keys[] = WIN_MACRO_11_KEYS;
      tap_key_sequence(keys, sizeof(keys) / sizeof(keys[0]));
    }
    return false;
#endif
#ifdef WIN_MACRO_12_KEYS
  case QK_MACRO_12:
    if (record->event.pressed) {
      static const uint16_t PROGMEM keys[] = WIN_MACRO_12_KEYS;
      tap_key_sequence(keys, sizeof(keys) / sizeof(keys[0]));
    }
    return false;
#endif

  case KC_SCR_SPD_INC:
    if (record->event.pressed) {
      user_config.scroll_threshold -= 5;
      if (user_config.scroll_threshold < 1) {
        user_config.scroll_threshold = 1;
      }
      eeconfig_update_user(user_config.raw);
      if (mouse_session_state != MOUSE_SESSION_NORMAL) {
        leave_mouse_session();
      }
    }
    return false;

  case KC_SCR_SPD_DEC:
    if (record->event.pressed) {
      user_config.scroll_threshold += 5;
      if (user_config.scroll_threshold > 200) {
        user_config.scroll_threshold = 200;
      }
      eeconfig_update_user(user_config.raw);
      if (mouse_session_state != MOUSE_SESSION_NORMAL) {
        leave_mouse_session();
      }
    }
    return false;

  case KC_URL_COPY:
    if (record->event.pressed) {
      // The press returns false, so the mouse layer can go off right away.
      mouse_session_force_normal(true);
      tap_code16(C(KC_L));
      tap_code16(C(KC_C));
      tap_code(KC_ESC);
    }
    return false;

  case KC_SCR_RST:
    if (record->event.pressed) {
      user_config.scroll_threshold = 30;
      eeconfig_update_user(user_config.raw);
      if (mouse_session_state != MOUSE_SESSION_NORMAL) {
        leave_mouse_session();
      }
    }
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
        if (scroll_key_held) {
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

  mouse_session_process_pointing(raw_movement);

  if (is_minizone_scroll_active() && (current_x != 0 || current_y != 0)) {
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

  mouse_report.x = current_x;
  mouse_report.y = current_y;
  mouse_report.h = current_h;
  mouse_report.v = current_v;
  keyball.last_mouse = mouse_report; // propagate post-processed values to OLED

  return mouse_report;
}

void housekeeping_task_user(void) {
  if (pending_dual_role) {
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

// CoreX TD(0): 英数 tap / window layer hold, judged at 250 ms.
uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
  return IS_QK_LAYER_TAP(keycode) && QK_LAYER_TAP_GET_TAP_KEYCODE(keycode) == KC_LNG2 ? 250 : TAPPING_TERM;
}

// Each profile has its own Shift + \\ toggle combo; the symbol combos run on both.
bool combo_should_trigger(uint16_t combo_index, combo_t *combo, uint16_t keycode, keyrecord_t *record) {
  if (combo_index == COMBO_TOGGLE_MAC) {
    return is_mac_profile();
  }
  if (combo_index == COMBO_TOGGLE_WIN) {
    return !is_mac_profile();
  }
  return true;
}

// True after the profile was switched by hand; replugging returns to OS detection.
static bool profile_fixed = false;
static char last_combo_symbol = 0;
static uint16_t last_combo_timer = 0;


// The profile toggle shares its keys with "|" (Shift + \\), so it fires only when held.
bool get_combo_must_hold(uint16_t combo_index, combo_t *combo) {
  return combo_index >= SYMBOL_COMBO_COUNT;
}

void process_combo_event(uint16_t combo_index, bool pressed) {
  static uint16_t held_symbol = KC_NO;
  if (combo_index >= SYMBOL_COMBO_COUNT) {
    if (pressed) {
      set_profile(!is_mac_profile());
      profile_fixed = true;
    }
    return;
  }
  if (!pressed) {
    unregister_code16(held_symbol);
    held_symbol = KC_NO;
    return;
  }
  // A combo is keyboard input, so it ends a mouse session like any other key.
  mouse_session_force_normal(true);
  // Held like a normal key so the symbol auto-repeats, as on CoreX.
  held_symbol = pgm_read_word(is_mac_profile() ? &combo_mac_output[combo_index] : &combo_win_output[combo_index]);
  register_code16(held_symbol);
  last_combo_symbol = pgm_read_byte(&combo_symbol[combo_index]);
  last_combo_timer = timer_read();
}

layer_state_t layer_state_set_user(layer_state_t ly_state)
{
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
  } else if (!mouse_layer_active && mouse_layer_held) {
    // かな is still held after a momentary layer (e.g. K hold) ends.
    ly_state |= (layer_state_t)1 << mouse_layer();
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

#ifdef OLED_ENABLE

#include "lib/oledkit/oledkit.h"

static const char *layer_name(uint8_t layer) {
  switch (layer % PROFILE_LAYERS) {
    case L_WIN_EDIT:
      return PSTR("EDIT");
    case L_WIN_MOUSE:
      return PSTR("MOUSE");
    case L_WIN_WINDOW:
      return PSTR("WINDOW");
    case L_WIN_NUM:
      return PSTR("NUM");
    default:
      return PSTR("BASE");
  }
}

static void write_spaces(uint8_t n) {
  while (n--) {
    oled_write_char(' ', false);
  }
}

// Master OLED, 21 x 4 characters. Every row is written at full width so stale
// characters never remain; the driver only redraws bytes that changed.
void oledkit_render_info_user(void) {
  bool mac = is_mac_profile();
  uint8_t layer = get_highest_layer(layer_state | default_layer_state);
  const char *name = layer_name(layer);
  uint8_t mods = get_mods();

  // Row 0: profile, layer, AUTO/FIXED
  oled_set_cursor(0, 0);
  oled_write_P(mac ? PSTR(" MAC ") : PSTR(" WIN "), true);
  oled_write_char(' ', false);
  oled_write_P(name, layer != L_WIN_BASE && layer != L_MAC_BASE);
  write_spaces(10 - strlen_P(name));
  oled_write_P(profile_fixed ? PSTR("FIXED") : PSTR(" AUTO"), false);

  // Row 1: held modifiers
  oled_set_cursor(0, 1);
  oled_write_char(' ', false);
  oled_write_P(PSTR("SFT"), mods & MOD_MASK_SHIFT);
  oled_write_char(' ', false);
  oled_write_P(PSTR("CTL"), mods & MOD_MASK_CTRL);
  oled_write_char(' ', false);
  oled_write_P(mac ? PSTR("OPT") : PSTR("ALT"), mods & MOD_MASK_ALT);
  oled_write_char(' ', false);
  oled_write_P(mac ? PSTR("CMD") : PSTR("WIN"), mods & MOD_MASK_GUI);
  write_spaces(5);

  // Row 2: mouse state and the symbol of the last combo (shown for 1 s)
  oled_set_cursor(0, 2);
  oled_write_P(PSTR("\xB2 "), false);
  if (is_minizone_scroll_active()) {
    oled_write_P(PSTR("SCROLL"), false);
  } else if (held_mouse_buttons != 0) {
    oled_write_P(PSTR("CLICK "), false);
  } else if (layer_state_is(mouse_layer())) {
    oled_write_P(PSTR("MOUSE "), false);
  } else {
    oled_write_P(PSTR("-     "), false);
  }
  write_spaces(10);
  if (last_combo_symbol && timer_elapsed(last_combo_timer) < 1000) {
    oled_write_char(' ', true);
    oled_write_char(last_combo_symbol, true);
    oled_write_char(' ', true);
  } else {
    last_combo_symbol = 0;  // stop the 16-bit timer from showing it again after wrap-around
    write_spaces(3);
  }

  // Row 3: ball speed and scroll threshold
  oled_set_cursor(0, 3);
  oled_write_P(PSTR("\xBC\xBD\xB1"), false);
  oled_write(get_u8_str(keyball_get_cpi(), ' '), false);
  oled_write_P(PSTR("00   \xBE\xBF\xB1"), false);
  oled_write(get_u8_str(user_config.scroll_threshold, ' '), false);
  write_spaces(4);
}
#endif
