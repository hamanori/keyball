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
  // Keep the values stable while existing Remap EEPROM profiles use User 0-10.
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
  KC_SCR_RST            = KEYBALL_SAFE_RANGE + 10  // User10 スクロール閾値を初期値に戻す
};

enum layers {
  L_SCROLL = 3,
  L_MOUSE = 6,
};

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
uint16_t auto_mouse_movement = 0;
uint32_t auto_mouse_movement_timer = 0;
bool invert_scroll = false; // OS判定でスクロール方向を反転する
#if HAS_OS_DETECTION
uint8_t cached_os = 0;      // 一度だけ判定して保持
#endif

static bool is_minizone_scroll_active(void) {
  return scroll_key_held || scroll_layer_active;
}

static void reset_scroll_counters(void) {
  scroll_v_mouse_interval_counter = 0;
  scroll_h_mouse_interval_counter = 0;
}

static void reset_auto_mouse_movement(void) {
  auto_mouse_movement = 0;
  auto_mouse_movement_timer = timer_read32();
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

  // The QMK auto mouse module is the sole owner of the temporary mouse layer.
  set_auto_mouse_enable(true);
  set_auto_mouse_timeout(AUTO_MOUSE_TIME);

#if HAS_OS_DETECTION
  // OS自動判定 (少し待ってから実行する必要あり)
  wait_ms(400);
  cached_os = detected_host_os();
  switch (cached_os) {
    case OS_WINDOWS:
    case OS_LINUX:
      invert_scroll = true;  // Windows/Linux用にスクロール方向を反転
      break;
    case OS_MACOS:
      invert_scroll = false; // macOS/iOSはそのまま
      break;
    case OS_IOS:
      invert_scroll = false; // macOS/iOSはそのまま
      break;
    default:
      invert_scroll = false;
      break;
  }
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

bool is_mouse_record_user(uint16_t keycode, keyrecord_t *record) {
  (void)record;

  return (keycode >= KC_MY_BTN1 && keycode <= KC_MY_BTN5) || keycode == KC_MY_SCR;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record)
{

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
    extern void register_mouse(uint8_t mouse_keycode, bool pressed);

    register_mouse(KC_MS_BTN1 + button_index, record->event.pressed);
    if (record->event.pressed) {
      held_mouse_buttons |= button_mask;
    } else {
      held_mouse_buttons &= ~button_mask;
    }
    return false;
  }

  case KC_MY_SCR:
    if (scroll_key_held != record->event.pressed) {
      scroll_key_held = record->event.pressed;
      reset_scroll_counters();
    }
    return false;

  case KC_TO_CLICKABLE_INC:
  case KC_TO_CLICKABLE_DEC:
    // Reserved until the existing Remap profiles are replaced by static maps.
    return false;

  case KC_SCR_SPD_INC:
    if (record->event.pressed) {
      user_config.scroll_threshold -= 5;
      if (user_config.scroll_threshold < 1) {
        user_config.scroll_threshold = 1;
      }
      eeconfig_update_user(user_config.raw);
    }
    return false;

  case KC_SCR_SPD_DEC:
    if (record->event.pressed) {
      user_config.scroll_threshold += 5;
      if (user_config.scroll_threshold > 200) {
        user_config.scroll_threshold = 200;
      }
      eeconfig_update_user(user_config.raw);
    }
    return false;

  case KC_SCR_RST:
    if (record->event.pressed) {
      user_config.scroll_threshold = 30;
      eeconfig_update_user(user_config.raw);
    }
    return false;

  default:
    break;
  }

  if (record->event.pressed) {
    if (held_keyboard_keys < UINT8_MAX) {
      held_keyboard_keys++;
    }
  } else if (held_keyboard_keys > 0) {
    held_keyboard_keys--;
  }

  // Keyboard input always wins over the temporary mouse layer. Reset on both
  // edges so activation starts from fresh movement after the final release.
  auto_mouse_reset_trigger(record->event.pressed);
  reset_auto_mouse_movement();

  return true;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report)
{
  int16_t current_x = mouse_report.x;
  int16_t current_y = mouse_report.y;
  int16_t current_h = mouse_report.h;
  int16_t current_v = mouse_report.v;

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

bool auto_mouse_activation(report_mouse_t mouse_report) {
  if (is_minizone_scroll_active()) {
    reset_auto_mouse_movement();
    return false;
  }

  if (mouse_report.buttons) {
    return true;
  }

  if (held_keyboard_keys > 0) {
    reset_auto_mouse_movement();
    return false;
  }

  uint16_t movement = my_abs(mouse_report.x) + my_abs(mouse_report.y);
  if (movement == 0) {
    return false;
  }

  if (timer_elapsed32(auto_mouse_movement_timer) > AUTO_MOUSE_ACTIVATION_WINDOW) {
    auto_mouse_movement = 0;
  }

  auto_mouse_movement_timer = timer_read32();
  auto_mouse_movement += movement;
  if (auto_mouse_movement < AUTO_MOUSE_ACTIVATION_DISTANCE) {
    return false;
  }

  auto_mouse_movement = 0;
  return true;
}

////////////////////////////////////
///
/// 自動マウスレイヤーの実装 ここまで
///
////////////////////////////////////

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // keymap for default
  [0] = LAYOUT_universal(
    KC_ESC   , KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                                        KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     , KC_DEL   ,
    KC_TAB   , KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                                        KC_H     , KC_J     , KC_K     , KC_L     , KC_SCLN  , S(KC_7)  ,
    KC_LSFT  , KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                                        KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  , KC_INT1  ,
              KC_LALT,KC_LGUI,LCTL_T(KC_LNG2)    ,LT(1,KC_SPC),LT(3,KC_LNG1),                  KC_BSPC,LT(2,KC_ENT), RCTL_T(KC_LNG2),     KC_RALT  , KC_PSCR
  ),

  [1] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
                  _______  , _______  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  ),

  [2] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
                  _______  , _______  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  ),

  [3] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
                  _______  , _______  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  ),

  [4] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
                  _______  , _______  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  ),

  [5] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , KC_SCR_SPD_INC , KC_SCR_SPD_DEC , KC_SCR_RST , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
                  _______  , _______  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  ),

  [6] = LAYOUT_universal(
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , _______  , _______  , _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  ,KC_MY_BTN1, KC_MY_SCR  ,KC_MY_BTN2, _______  , _______  ,
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , KC_MY_BTN4  , _______  , KC_MY_BTN5  , _______  , _______  ,
                  _______  , _______  , _______  ,        _______  , _______  ,                   _______  , _______  , _______       , _______  , _______
  )
};
// clang-format on

layer_state_t layer_state_set_user(layer_state_t ly_state)
{
  bool next_scroll_layer_active = (ly_state & ((layer_state_t)1 << L_SCROLL)) != 0;

  // Layer 3 owns ball motion, so do not allow the auto mouse layer to overlap it.
  if (next_scroll_layer_active) {
    ly_state &= ~((layer_state_t)1 << L_MOUSE);
  }

  if (scroll_layer_active != next_scroll_layer_active) {
    scroll_layer_active = next_scroll_layer_active;
    reset_scroll_counters();
  }
  keyball_set_scroll_mode(false);

  return ly_state;
}

// Provide a short label for the current click/scroll state to show on OLED.
const char *keyball_get_state_label(void) {
  if (is_minizone_scroll_active()) {
    return "SCRL";
  }
  if (held_mouse_buttons != 0) {
    return "CLCK";
  }
  if (layer_state_is(L_MOUSE)) {
    return "CLKB";
  }
  return "NONE";
}

#ifdef OLED_ENABLE

#include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void)
{
  // Ensure we start rendering from the top-left.
  oled_set_cursor(0, 0);
  oled_write_P(PSTR("Info\xB1"), false);
  oled_write_P(PSTR("LY:"), false);
  oled_write(get_u8_str(get_highest_layer(layer_state), ' '), false);
  oled_set_cursor(12, 0);
  oled_write_P(PSTR(" "), false);
  oled_write(keyball_get_state_label(), false);

  // Show CPI only (Ball info removed)
  oled_set_cursor(0, 1);
  oled_write_P(PSTR("\xBC\xBD\xB1"), false); // CPI label with custom font
  oled_write(get_u8_str(keyball_get_cpi(), ' '), false);
  oled_write_P(PSTR("00 "), false);

  oled_set_cursor(0, 2);
  oled_write_P(PSTR("    \xB1"), false);
  oled_write_P(PSTR("ST:"), false);
  oled_write(get_u8_str(user_config.scroll_threshold, ' '), false);
  oled_set_cursor(12, 2);
  oled_write_P(PSTR(" "), false);
  oled_write_P(PSTR("OS:"), false);
#if HAS_OS_DETECTION
  switch (cached_os) {
    case OS_WINDOWS:
      oled_write_P(PSTR("WIN"), false);
      break;
    case OS_MACOS:
      oled_write_P(PSTR("MAC"), false);
      break;
    case OS_LINUX:
      oled_write_P(PSTR("LNX"), false);
      break;
    case OS_IOS:
      oled_write_P(PSTR("IOS"), false);
      break;
    default:
      oled_write_P(PSTR("UNK"), false);
      break;
  }
#else
  oled_write_P(PSTR("NA"), false);
#endif
}
#endif
