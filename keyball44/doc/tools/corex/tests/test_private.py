import json
from pathlib import Path
import subprocess
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / "prepare_private.py"


class PrivateBuildInputTests(unittest.TestCase):
    def run_export(self, entries):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source, target = root / "backup.json", root / "secrets.h"
            source.write_text(json.dumps({"macros": {"entries": entries}}))
            result = subprocess.run(
                ["uv", "run", "--no-project", "python", str(SCRIPT), str(source), str(target)],
                capture_output=True, text=True,
            )
            return result, target.read_text() if target.exists() else None, target.stat().st_mode & 0o777 if target.exists() else None

    def test_exports_only_two_ascii_payloads_without_printing_them(self):
        first, second = b"fixture@example.test", b"Fixture42"
        result, header, mode = self.run_export([first.hex(), second.hex(), "unused"])
        self.assertEqual(result.returncode, 0)
        self.assertIn("COREX_PRIVATE_1", header)
        self.assertIn("COREX_PRIVATE_2", header)
        self.assertIn("0x021F", header)  # US @ is Shift+2; runtime translates it for JIS.
        self.assertNotIn(first.decode(), result.stdout + result.stderr)
        self.assertNotIn(first.hex(), result.stdout + result.stderr)
        self.assertEqual(mode, 0o600)

    def test_rejects_vial_commands_without_creating_header(self):
        result, header, _ = self.run_export(["010104", "4142"])
        self.assertNotEqual(result.returncode, 0)
        self.assertIsNone(header)


if __name__ == "__main__":
    unittest.main()
