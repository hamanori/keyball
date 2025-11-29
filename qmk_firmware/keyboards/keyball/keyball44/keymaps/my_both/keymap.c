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

static const char *format_4u(uint16_t v) {
  // Right-aligned 4-digit decimal (space padded).
  static char buf[5] = {' ', ' ', ' ', ' ', 0};
  buf[3] = (v % 10) + '0';
  v /= 10;
  buf[2] = v ? (v % 10) + '0' : ' ';
  v /= 10;
  buf[1] = v ? (v % 10) + '0' : ' ';
  v /= 10;
  buf[0] = v ? (v % 10) + '0' : ' ';
  return buf;
}

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
  KC_MY_BTN1 = KEYBALL_SAFE_RANGE, // User0
  KC_MY_BTN2,                      // User1
  KC_MY_BTN3,                      // User2
  KC_MY_BTN4,                      // User3
  KC_MY_BTN5,                      // User4
  KC_MY_SCR,                       // User5             
  KC_TO_CLICKABLE_INC,             // User6
  KC_TO_CLICKABLE_DEC,             // User7 
  KC_SCR_SPD_INC,                  // User8 スクロール閾値を下げて速度アップ
  KC_SCR_SPD_DEC                   // User9 スクロール閾値を上げて速度ダウン
};

enum click_state {
  NONE = 0,
  WAITING,    // マウスレイヤーが有効になるのを待つ。 Wait for mouse layer to activate.
  CLICKABLE,  // マウスレイヤー有効になりクリック入力が取れる。 Mouse layer is enabled to take click input.
  CLICKING,   // クリック中。 Clicking.
  SCROLLING   // スクロール中。 Scrolling.
};

typedef union {
  uint32_t raw;
  struct {
    // int16_t to_clickable_time; // // この秒数(千分の一秒)、WAITING状態ならクリックレイヤーが有効になる。  For this number of seconds (milliseconds), if in WAITING state, the click layer is activated.
    int16_t to_clickable_movement;
    int16_t scroll_threshold;
  };
} user_config_t;

user_config_t user_config;

enum click_state state; // 現在のクリック入力受付の状態 Current click input reception status
uint16_t click_timer;   // タイマー。状態に応じて時間で判定する。 Timer. Time to determine the state of the system.

// uint16_t to_clickable_time = 50;   // この秒数(千分の一秒)、WAITING状態ならクリックレイヤーが有効になる。  For this number of seconds (milliseconds), if in WAITING state, the click layer is activated.
uint16_t to_reset_time = 5000; // この秒数(千分の一秒)、CLICKABLE状態ならクリックレイヤーが無効になる。 For this number of seconds (milliseconds), the click layer is disabled if in CLICKABLE state.

const uint16_t click_layer = 6; // マウス入力が可能になった際に有効になるレイヤー。Layers enabled when mouse input is enabled

int16_t scroll_v_mouse_interval_counter; // 垂直スクロールの入力をカウントする。　Counting Vertical Scroll Inputs
int16_t scroll_h_mouse_interval_counter; // 水平スクロールの入力をカウントする。  Counts horizontal scrolling inputs.

int16_t after_click_lock_movement = 0; // クリック入力後の移動量を測定する変数。 Variable that measures the amount of movement after a click input.

int16_t mouse_record_threshold = 30; // ポインターの動きを一時的に記録するフレーム数。 Number of frames in which the pointer movement is temporarily recorded.
int16_t mouse_move_count_ratio = 5;  // ポインターの動きを再生する際の移動フレームの係数。 The coefficient of the moving frame when replaying the pointer movement.

int16_t mouse_movement;
bool invert_scroll = false; // OS判定でスクロール方向を反転する
#if HAS_OS_DETECTION
uint8_t cached_os = 0;      // 一度だけ判定して保持
#endif

void eeconfig_init_user(void) {
  user_config.raw = 0;
  user_config.to_clickable_movement = 50; // user_config.to_clickable_time = 10;
  user_config.scroll_threshold = 50;
  eeconfig_update_user(user_config.raw);
}

void keyboard_post_init_user(void) {
  user_config.raw = eeconfig_read_user();
  if (user_config.to_clickable_movement < 5) {
    user_config.to_clickable_movement = 50;
    eeconfig_update_user(user_config.raw);
  }
  if (user_config.scroll_threshold < 1) {
    user_config.scroll_threshold = 50;
    eeconfig_update_user(user_config.raw);
  }

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
    case OS_IOS:
      invert_scroll = false; // macOS/iOSはそのまま
      break;
    default:
      invert_scroll = false;
      break;
  }
#endif
}

// クリック用のレイヤーを有効にする。　Enable layers for clicks
void enable_click_layer(void)
{
  layer_on(click_layer);
  click_timer = timer_read();
  state = CLICKABLE;
}

// クリック用のレイヤーを無効にする。 Disable layers for clicks.
void disable_click_layer(void)
{
  state = NONE;
  layer_off(click_layer);
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

// 自前の符号を返す関数。 Function to return the sign.
int16_t mmouse_move_y_sign(int16_t num)
{
  if (num < 0)
  {
    return -1;
  }

  return 1;
}

// 現在クリックが可能な状態か。 Is it currently clickable?
bool is_clickable_mode(void)
{
  return state == CLICKABLE || state == CLICKING || state == SCROLLING;
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
    report_mouse_t currentReport = pointing_device_get_report();

    // どこのビットを対象にするか。 Which bits are to be targeted?
    uint8_t btn = 1 << (keycode - KC_MY_BTN1);

    if (record->event.pressed)
    {
      // ビットORは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットのどちらかが「1」の場合に「1」にします。
      // Bit OR compares bits in the same position on the left and right sides of the operator and sets them to "1" if either of both bits is "1".
      currentReport.buttons |= btn;
      state = CLICKING;
      after_click_lock_movement = 30;
    }
    else
    {
      // ビットANDは演算子の左辺と右辺の同じ位置にあるビットを比較して、両方のビットが共に「1」の場合だけ「1」にします。
      // Bit AND compares the bits in the same position on the left and right sides of the operator and sets them to "1" only if both bits are "1" together.
      currentReport.buttons &= ~btn;
      enable_click_layer();
    }

    pointing_device_set_report(currentReport);
    pointing_device_send();
    return false;
  }

  case KC_MY_SCR:
    if (record->event.pressed) {
      state = SCROLLING;
    } else {
      enable_click_layer(); // スクロールキーを離した時に再度クリックレイヤーを有効にする。 Enable click layer again when the scroll key is released.
    }
    return false;

  case KC_TO_CLICKABLE_INC:
    if (record->event.pressed) {
      user_config.to_clickable_movement += 5; // user_config.to_clickable_time += 10;
      eeconfig_update_user(user_config.raw);
    }
    return false;

  case KC_TO_CLICKABLE_DEC:
    if (record->event.pressed) {
      user_config.to_clickable_movement -= 5; // user_config.to_clickable_time -= 10;

      if (user_config.to_clickable_movement < 5) {
        user_config.to_clickable_movement = 5;
      }

      // if (user_config.to_clickable_time < 10) {
      //     user_config.to_clickable_time = 10;
      // }

      eeconfig_update_user(user_config.raw);
    }
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

  default:
    if (record->event.pressed)
    {
      disable_click_layer();
    }
  }

  return true;
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report)
{
  int16_t current_x = mouse_report.x;
  int16_t current_y = mouse_report.y;
  int16_t current_h = mouse_report.h;
  int16_t current_v = mouse_report.v;

  if (current_x != 0 || current_y != 0 || current_h != 0 || current_v != 0)
  {

    switch (state)
    {
    case CLICKABLE:
      click_timer = timer_read();
      break;

    case CLICKING:
      after_click_lock_movement -= my_abs(current_x) + my_abs(current_y);

      if (after_click_lock_movement > 0) {
        current_x = 0;
        current_y = 0;
      }

      break;

    case SCROLLING:
    {
      int8_t rep_v = 0;
      int8_t rep_h = 0;
      int16_t sv_th = user_config.scroll_threshold < 1 ? 1 : user_config.scroll_threshold;
      int16_t sh_th = sv_th;

      // 垂直スクロールの方の感度を高める。 Increase sensitivity toward vertical scrolling.
      if (my_abs(current_y) * 2 > my_abs(current_x)) {

        scroll_v_mouse_interval_counter += current_y;
        while (my_abs(scroll_v_mouse_interval_counter) > sv_th) {
          if (scroll_v_mouse_interval_counter < 0) {
            scroll_v_mouse_interval_counter += sv_th;
            rep_v += sv_th;
          } else {
            scroll_v_mouse_interval_counter -= sv_th;
            rep_v -= sv_th;
          }

        }
      } else {

        scroll_h_mouse_interval_counter += current_x;

        while (my_abs(scroll_h_mouse_interval_counter) > sh_th) {
          if (scroll_h_mouse_interval_counter < 0) {
            scroll_h_mouse_interval_counter += sh_th;
            rep_h += sh_th;
          } else {
            scroll_h_mouse_interval_counter -= sh_th;
            rep_h -= sh_th;
          }
        }
      }

      current_h = rep_h / sh_th;
      current_v = -rep_v / sv_th;
      if (invert_scroll) {
        current_h = -current_h;
        current_v = -current_v;
      }
      current_x = 0;
      current_y = 0;
    }
    break;

    case WAITING:
      /*
      if (timer_elapsed(click_timer) > user_config.to_clickable_time) {
          enable_click_layer();
      }
      */

      mouse_movement += my_abs(current_x) + my_abs(current_y) + my_abs(current_h) + my_abs(current_v);

      if (mouse_movement >= user_config.to_clickable_movement)
      {
        mouse_movement = 0;
        enable_click_layer();
      }
      break;

    default:
      click_timer = timer_read();
      state = WAITING;
      mouse_movement = 0;
    }
  }
  else
  {
    switch (state)
    {
    case CLICKING:
    case SCROLLING:

      break;

    case CLICKABLE:
      if (timer_elapsed(click_timer) > to_reset_time) {
        disable_click_layer();
      }
      break;

    case WAITING:
      if (timer_elapsed(click_timer) > 50) {
        mouse_movement = 0;
        state = NONE;
      }
      break;

    default:
      mouse_movement = 0;
      state = NONE;
    }
  }

  mouse_report.x = current_x;
  mouse_report.y = current_y;
  mouse_report.h = current_h;
  mouse_report.v = current_v;

  return mouse_report;
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
              KC_LALT,KC_LGUI,LCTL_T(KC_LNG2)     ,LT(1,KC_SPC),LT(3,KC_LNG1),                  KC_BSPC,LT(2,KC_ENT), RCTL_T(KC_LNG2),     KC_RALT  , KC_PSCR
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
    _______  , _______  , _______  , _______  , _______  , _______  ,                                        _______  , _______  , KC_SCR_SPD_INC , KC_SCR_SPD_DEC , _______  , _______  ,
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
  uint8_t highest = get_highest_layer(ly_state);

  // レイヤー3はminiZoneスクロールを使うため、Keyball標準スクロールを無効化し状態をSCROLLINGにセット
  if (highest == 3) {
    keyball_set_scroll_mode(false); // x/yを保持
    disable_click_layer();
    state = SCROLLING;
    scroll_v_mouse_interval_counter = 0;
    scroll_h_mouse_interval_counter = 0;
    click_timer = timer_read();
  } else {
    // レイヤー3を抜けたらminiZoneスクロール状態を解除
    if (state == SCROLLING) {
      state = NONE;
      scroll_v_mouse_interval_counter = 0;
      scroll_h_mouse_interval_counter = 0;
      after_click_lock_movement = 0;
      mouse_movement = 0;
    }
    keyball_set_scroll_mode(false); // 他レイヤーは通常ポインタ
  }

  return ly_state;
}

#ifdef OLED_ENABLE

#include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void)
{
  keyball_oled_render_ballinfo();

  oled_set_cursor(0, 2);
  oled_write_P(PSTR("LYR \xB1"), false);
  oled_write(format_4u(get_highest_layer(layer_state)), false);
  oled_write_char(' ', false);
  oled_write_P(PSTR("MV  \xB1"), false);
  oled_write(format_4u(mouse_movement), false);
  oled_write_char('/', false);
  oled_write(format_4u(user_config.to_clickable_movement), false);
  oled_write_P(PSTR("   "), false); // clear remainder if digits shrink

  oled_set_cursor(0, 3);
  oled_write_P(PSTR("ST  \xB1"), false);
  oled_write(format_4u(user_config.scroll_threshold), false);
  oled_write_char(' ', false);
  oled_write_P(PSTR("OS  \xB1"), false);
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
  oled_write_P(PSTR("   "), false); // clear remainder if text shrinks
}
#endif
