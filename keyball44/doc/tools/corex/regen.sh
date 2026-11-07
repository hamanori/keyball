#!/bin/sh
# Regenerate the Keyball44 specs, firmware include, and Mac/Windows cheat sheet.
set -eu

if [ "$#" -gt 1 ]; then
  echo "usage: regen.sh [OUTPUT_ROOT]" >&2
  exit 2
fi

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../../../.." && pwd)
OUTPUT_ROOT=${1:-$ROOT}
SNAPSHOT="$ROOT/keyball44/doc/reference/corex/corex-v0.13.0-20261106.json"
REFERENCE="$OUTPUT_ROOT/keyball44/doc/reference/corex"
KEYMAP="$OUTPUT_ROOT/qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori_corex/corex_keymaps.inc"
HTML="$OUTPUT_ROOT/keyball44/doc/keyball44-cheatsheet.html"

mkdir -p "$REFERENCE" "$(dirname -- "$KEYMAP")" "$(dirname -- "$HTML")"
uv run --no-project python "$SCRIPT_DIR/mac_spec.py" "$SNAPSHOT" "$REFERENCE/mac-spec.json"
uv run --no-project python "$SCRIPT_DIR/win_spec.py" "$SNAPSHOT" "$REFERENCE/win-spec.json"
uv run --no-project python "$SCRIPT_DIR/gen_firmware.py" "$REFERENCE/mac-spec.json" "$REFERENCE/win-spec.json" "$KEYMAP"
uv run --no-project python "$SCRIPT_DIR/gen_html.py" "$REFERENCE/mac-spec.json" "$REFERENCE/win-spec.json" "$HTML"
