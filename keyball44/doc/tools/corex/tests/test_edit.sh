#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
keymap_dir="$script_dir/../../../../../qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori_corex"
binary=$(mktemp /tmp/corex-edit-test.XXXXXX)
trap 'rm -f "$binary"' EXIT HUP INT TERM

${CC:-cc} -std=c99 -Wall -Wextra -Werror -pedantic \
    -I "$keymap_dir" \
    "$keymap_dir/corex_edit.c" "$script_dir/test_edit.c" \
    -o "$binary"
"$binary"
