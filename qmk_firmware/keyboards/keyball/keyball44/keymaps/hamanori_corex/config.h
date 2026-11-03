/*
This is the c configuration file for the keymap

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

#pragma once

// #ifdef RGBLIGHT_ENABLE
// #    define RGBLIGHT_EFFECT_BREATHING
// #    define RGBLIGHT_EFFECT_RAINBOW_MOOD
// #    define RGBLIGHT_EFFECT_RAINBOW_SWIRL
// #    define RGBLIGHT_EFFECT_SNAKE
// #    define RGBLIGHT_EFFECT_KNIGHT
// #    define RGBLIGHT_EFFECT_CHRISTMAS
// #    define RGBLIGHT_EFFECT_STATIC_GRADIENT
// #    define RGBLIGHT_EFFECT_RGB_TEST
// #    define RGBLIGHT_EFFECT_ALTERNATING
// #    define RGBLIGHT_EFFECT_TWINKLE
// #endif

/* タップ系の微調整（そのまま） */
#define TAP_CODE_DELAY 5

// Mouse session. Layer 6 is owned by the keymap state machine instead of
// QMK's built-in Auto Mouse lifecycle.
#define MOUSE_SESSION_READY_TIMEOUT 10000
#define MOUSE_SESSION_MULTI_CLICK_TERM 500
#define MOUSE_SESSION_ACTIVATION_DISTANCE 6
#define MOUSE_SESSION_ACTIVATION_WINDOW 100
#define MOUSE_SESSION_DRAG_THRESHOLD 6
#define MOUSE_SESSION_BALL_STOP_REPORTS 2

// Tapping term
#define TAPPING_TERM 200
#define HOLD_ON_OTHER_KEY_PRESS_PER_KEY
#define PERMISSIVE_HOLD_PER_KEY
#define TAPPING_TERM_PER_KEY

//  quick tap term
// This is the time to wait for a quick tap before sending a hold event.
#define QUICK_TAP_TERM 0

// Increase the number of layers to 7

#define KEYBALL_CPI_DEFAULT 700      // 光学センサーPMW3360DM の解像度 (CPI) の規定値
#define KEYBALL_SCROLL_DIV_DEFAULT 4 // スクロール速度の規定値

// Layers 0-11 (Windows 0-6, macOS 7-11).
#undef LAYER_STATE_8BIT
#define LAYER_STATE_16BIT

// CoreX combos
#define COMBO_TERM 30
#define COMBO_SHOULD_TRIGGER

// No one-shot keys in either profile.
#define NO_ACTION_ONESHOT
