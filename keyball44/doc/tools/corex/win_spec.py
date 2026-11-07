"""Write the Windows profile spec from the sanitized CoreX snapshot."""

import json
from pathlib import Path
import sys

from corex_generation import build_specs, load_snapshot


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: win_spec.py COREX_SNAPSHOT.json OUT_SPEC.json")
    _, win = build_specs(load_snapshot(Path(sys.argv[1])))
    Path(sys.argv[2]).write_text(json.dumps(win, ensure_ascii=False, indent=2) + "\n")


if __name__ == "__main__":
    main()
