"""End-to-end checks for the committed, sanitized CoreX keymap source."""

import json
from pathlib import Path
import subprocess
import tempfile
import unittest


TOOLS = Path(__file__).resolve().parents[1]
REGEN = TOOLS / "regen.sh"
SOURCE = TOOLS.parents[1] / "reference" / "corex" / "corex-v0.13.0-20261106.json"


class GenerationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory()
        cls.output_root = Path(cls.temporary.name)
        subprocess.run([str(REGEN), str(cls.output_root)], check=True, cwd=TOOLS)
        cls.reference = cls.output_root / "keyball44/doc/reference/corex"
        cls.keymap_include = cls.output_root / (
            "qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori_corex/corex_keymaps.inc"
        )
        cls.html = cls.output_root / "keyball44/doc/keyball44-cheatsheet.html"
        cls.mac = json.loads((cls.reference / "mac-spec.json").read_text())
        cls.win = json.loads((cls.reference / "win-spec.json").read_text())

    @classmethod
    def tearDownClass(cls):
        cls.temporary.cleanup()

    def test_regeneration_writes_both_profiles_firmware_include_and_html(self):
        self.assertTrue(SOURCE.is_file())
        self.assertTrue(self.keymap_include.is_file())
        self.assertTrue(self.html.is_file())
        self.assertEqual(self.mac["layerIds"], {
            "base": 6, "edit": 7, "symbol": 8, "num": 9, "mouse": 10, "settings": 11,
        })
        self.assertEqual(self.win["layerIds"], {
            "base": 0, "edit": 1, "symbol": 2, "num": 3, "mouse": 4, "settings": 5,
        })

    def test_layer_taps_follow_the_physical_tab_quote_kana_enter_and_eisu_keys(self):
        expected = {
            ("base", "1,0"): (0x412B, 0x472B),  # Tab holds edit.
            ("base", "5,0"): (0x4134, 0x4734),  # Quote holds edit.
            ("base", "3,5"): (0x4290, 0x4890),  # Kana holds symbol.
            ("base", "7,4"): (0x4228, 0x4828),  # Enter holds symbol.
            ("base", "3,3"): (0x4391, 0x4991),  # Eisu holds numbers.
        }
        for (layer, position), (win_code, mac_code) in expected.items():
            with self.subTest(position=position):
                self.assertEqual(self.win["layers"][layer][position]["code"], win_code)
                self.assertEqual(self.mac["layers"][layer][position]["code"], mac_code)

    def test_mouse_edit_and_settings_actions_keep_their_approved_codes(self):
        self.assertEqual(self.mac["layers"]["mouse"]["5,4"]["code"], 0x7E40)  # J: left click.
        self.assertEqual(self.mac["layers"]["mouse"]["5,2"]["code"], 0x7E41)  # L: right click.
        self.assertEqual(self.win["layers"]["mouse"]["5,3"]["code"], 0x43D3)  # K: middle click / num.
        self.assertEqual(self.mac["layers"]["mouse"]["5,3"]["code"], 0x49D3)
        self.assertEqual(self.mac["layers"]["edit"]["0,2"]["code"], 0x7E74)  # W: cut all.
        self.assertEqual(self.win["layers"]["edit"]["2,4"]["code"], 0x7E7D)  # V: paste.
        self.assertEqual(self.mac["layers"]["settings"]["0,0"]["code"], 0x00A9)
        self.assertEqual(self.mac["layers"]["settings"]["1,0"]["code"], 0x00AA)
        self.assertEqual(self.mac["layers"]["settings"]["2,0"]["code"], 0x00A8)
        self.assertEqual(self.mac["layers"]["settings"]["4,1"]["code"], 0x006B)

    def test_settings_combo_and_two_private_combos_are_the_only_combo_outputs(self):
        expected = [
            ("COMBO_SETTINGS", ["2,0", "6,0"]),
            ("COMBO_PRIVATE_1", ["1,1", "1,2", "1,3", "1,4"]),
            ("COMBO_PRIVATE_2", ["5,4", "5,3", "5,2", "5,1"]),
        ]
        for spec in (self.mac, self.win):
            self.assertEqual([(combo["id"], combo["positions"]) for combo in spec["combos"]], expected)
        firmware = self.keymap_include.read_text()
        self.assertIn("enum { COMBO_SETTINGS, COMBO_PRIVATE_1, COMBO_PRIVATE_2 };", firmware)
        self.assertIn("combo_t key_combos[]", firmware)
        self.assertEqual(firmware.count("COMBO_ACTION("), 3)
        self.assertNotIn("SYMBOL_COMBO_COUNT", firmware)

    def test_source_snapshot_and_cheat_sheet_never_include_private_string_contents(self):
        snapshot = json.loads(SOURCE.read_text())
        self.assertEqual(set(snapshot), {"source", "keymap", "combos"})
        self.assertNotIn("macros", snapshot)
        page = self.html.read_text()
        for label in ("通常", "編集・移動", "記号", "数字", "マウス", "設定"):
            self.assertIn(label, page)
        self.assertIn("文字列の内容は表示しません", page)
        self.assertNotIn("0x7E", page)


if __name__ == "__main__":
    unittest.main()
