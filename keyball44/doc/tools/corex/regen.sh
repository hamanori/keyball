#!/bin/sh
# Regenerate the hamanori_corex static keymaps and the cheat sheet from the saved inputs.
set -eu
cd "$(dirname "$0")"
REF=../../reference/corex
KM=../../../../qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori_corex
python3 mac_spec.py "$REF/corex-mac-20261103.json" "$REF/corex-layer-notes-20261103.json" "$REF/mac-spec.json"
python3 gen_firmware.py "$REF/mac-spec.json" "$REF/keyball-windows-remap-20261103.json" "$KM/corex_keymaps.inc"
python3 gen_html.py "$REF/mac-spec.json" ../../keyball44-mac-cheatsheet.html
