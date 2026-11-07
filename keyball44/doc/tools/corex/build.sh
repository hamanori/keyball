#!/bin/sh
# Build this checkout's keymap with an existing QMK toolchain, writing only to OUT.
# Usage: build.sh /path/to/qmk_firmware /private/output/directory
set -eu
QMK_ROOT=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
PORT_ROOT=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
KEYMAP="$PORT_ROOT/qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori_corex"
cd "$QMK_ROOT"
# The hex target omits QMK's cpfirmware step, preserving the QMK checkout's files.
make -f builddefs/build_keyboard.mk -j2 \
  KEYBOARD=keyball/keyball44 KEYMAP=hamanori_corex SILENT=false COLOR=false \
  MAIN_KEYMAP_PATH_1="$KEYMAP" BUILD_DIR="$OUT" TARGET=keyball_corex_v0130 hex
bytes=$(avr-size -A "$OUT/keyball_corex_v0130.elf" | awk '$1 == ".text" || $1 == ".data" { size += $2 } END { print size }')
test "$bytes" -le 28672 || { echo "Firmware exceeds Caterina capacity: $bytes / 28672 bytes" >&2; exit 1; }
printf 'Firmware size: %s / 28672 bytes\n' "$bytes"
