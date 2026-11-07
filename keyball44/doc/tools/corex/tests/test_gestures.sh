#!/bin/sh
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
keymap_dir="$script_dir/../../../../../qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori_corex"
binary=$(mktemp /tmp/corex-gesture-test.XXXXXX)
trap 'rm -f "$binary"' EXIT HUP INT TERM
${CC:-cc} -std=c99 -Wall -Wextra -Werror -I "$keymap_dir" \
  "$script_dir/test_gestures.c" "$keymap_dir/corex_gesture.c" -o "$binary"
"$binary"
