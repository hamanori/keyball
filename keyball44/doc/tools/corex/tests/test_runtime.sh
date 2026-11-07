#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root="$script_dir/../../../../.."
keymap_dir="$repo_root/qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori_corex"
stub_dir="$script_dir/runtime_stub"
qmk_root=${QMK_ROOT:-$HOME/qmk_firmware}
binary=$(mktemp /tmp/corex-runtime-test.XXXXXX)
trap 'rm -f "$binary"' EXIT HUP INT TERM

${CC:-cc} -std=gnu11 -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable \
    -DCOREX_HOST_TEST -DQMK_KEYBOARD_H='"quantum.h"' \
    -I "$stub_dir" -I "$keymap_dir" \
    -I "$qmk_root/quantum" -I "$qmk_root/quantum/keymap_extras" -I "$qmk_root/quantum/sequencer" \
    "$keymap_dir/keymap.c" "$keymap_dir/corex_edit.c" "$keymap_dir/corex_gesture.c" \
    "$script_dir/test_runtime.c" -o "$binary"
"$binary"
