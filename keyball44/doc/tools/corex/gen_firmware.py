"""Generate the static Keyball44 keymaps and combo ABI from the profile specs."""

import json
from pathlib import Path
import sys

from corex_generation import render_firmware


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: gen_firmware.py MAC_SPEC.json WIN_SPEC.json OUT.inc")
    mac = json.loads(Path(sys.argv[1]).read_text())
    win = json.loads(Path(sys.argv[2]).read_text())
    Path(sys.argv[3]).write_text(render_firmware(mac, win))


if __name__ == "__main__":
    main()
