"""Export only the two private text combos to a local, ignored QMK header."""
import argparse
import json
import os
from pathlib import Path
import tempfile


def us_key(byte):
    char = chr(byte)
    if "a" <= char <= "z":
        return byte - ord("a") + 4
    if "A" <= char <= "Z":
        return 0x0200 | (byte - ord("A") + 4)
    plain = "1234567890-=[]\\;'`,./ "
    codes = [*range(0x1E, 0x28), 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x2C]
    if char in plain:
        return codes[plain.index(char)]
    shifted = '!@#$%^&*()_+{}|:"~<>?'
    return 0x0200 | codes[shifted.index(char)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("backup", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    entries = json.loads(args.backup.read_text())["macros"]["entries"]
    if len(entries) < 2:
        parser.error("Two private text macros are required")
    lines = ["#pragma once", "// Private build input. Never commit this file."]
    for i, encoded in enumerate(entries[:2], 1):
        try:
            data = bytes.fromhex(encoded)
        except (ValueError, TypeError):
            parser.error("Invalid private macro encoding")
        if not data or not all(32 <= byte < 127 for byte in data):
            parser.error("Only nonempty printable ASCII text macros are supported")
        keys = ", ".join(f"0x{us_key(byte):04X}" for byte in data)
        lines.append(f"#define COREX_PRIVATE_{i} {{{keys}}}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(dir=args.output.parent)
    try:
        with os.fdopen(fd, "w") as stream:
            stream.write("\n".join(lines) + "\n")
        os.replace(temporary, args.output)
    finally:
        Path(temporary).unlink(missing_ok=True)
    print("Prepared two private combo payloads; contents omitted.")


if __name__ == "__main__":
    main()
