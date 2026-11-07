"""Generate a portable Keyball44 map from the sanitized CoreX v0.13 snapshot."""

from __future__ import annotations

import json
from pathlib import Path


KEYBALL_ROWS = 8
KEYBALL_COLS = 6
STUDIO_ROWS = 8
STUDIO_COLS = 7
USER_BASE = 0x7E40
LAYER_ORDER = ("base", "edit", "symbol", "num", "mouse", "settings")
PROFILE_LAYERS = {
    "win": dict(zip(LAYER_ORDER, range(0, 6))),
    "mac": dict(zip(LAYER_ORDER, range(6, 12))),
}
LAYER_INFO = {
    "base": ("通常", "文字入力"),
    "edit": ("編集・移動", "Tab を長押し"),
    "symbol": ("記号", "かなを長押し"),
    "num": ("数字", "英数を長押し"),
    "mouse": ("マウス", "ボールを動かすと入る／かなを長押し"),
    "settings": ("設定", "左 Shift ＋ \\ を押している間だけ有効"),
}


def _studio_position(value: str) -> tuple[int, int]:
    row, col = value.split(",")
    return int(row), int(col)


def _keyball_positions() -> dict[tuple[int, int], tuple[int, int]]:
    """Return the reviewed Keyball matrix position -> CoreX Studio position map."""
    positions: dict[tuple[int, int], tuple[int, int]] = {}
    for col in range(6):
        positions[(0, col)] = (4, col)
        positions[(1, col)] = (5, col)
        positions[(2, col)] = (6, col)

    right = (
        ((0, 0), (0, 1), (0, 2), (0, 3), (0, 4), (3, 4)),
        ((1, 0), (1, 1), (1, 2), (1, 3), (1, 4), (1, 5)),
        ((2, 0), (2, 1), (2, 2), (2, 3), (2, 4), (2, 5)),
    )
    for row, line in enumerate(right):
        for col, studio in enumerate(line):
            positions[(4 + row, col)] = studio

    positions.update({
        (3, 1): (7, 1),  # extra thumb: Option / Alt
        (3, 2): (7, 2),  # extra thumb: Command / Win
        (3, 3): (7, 3),
        (3, 4): (7, 4),
        (3, 5): (7, 5),
        (7, 1): (7, 0),  # extra thumb: Control / Ctrl
        (7, 4): (3, 3),
        (7, 5): (3, 5),
    })
    if len(positions) != 44:
        raise ValueError(f"Expected 44 Keyball positions, got {len(positions)}")
    return positions


KEYBALL_POSITIONS = _keyball_positions()

KEY_NAMES = {
    0x28: "Enter", 0x29: "Esc", 0x2A: "⌫", 0x2B: "Tab", 0x2C: "Space",
    0x2D: "-", 0x2E: "=", 0x2F: "[", 0x30: "]", 0x31: "\\", 0x33: ";",
    0x34: "'", 0x35: "`", 0x36: ",", 0x37: ".", 0x38: "/", 0x39: "Caps",
    0x46: "Print Screen", 0x47: "Scroll Lock", 0x48: "Pause", 0x49: "Insert",
    0x4A: "Home", 0x4B: "Page Up", 0x4C: "Delete", 0x4D: "End", 0x4E: "Page Down",
    0x4F: "→", 0x50: "←", 0x51: "↓", 0x52: "↑",
    0x68: "F13", 0x69: "F14", 0x6A: "F15", 0x6B: "F16", 0x6C: "F17",
    0x6D: "F18", 0x6E: "F19", 0x6F: "F20", 0x70: "F21", 0x71: "F22",
    0x72: "F23", 0x73: "F24", 0x88: "変換", 0x89: "無変換", 0x90: "かな",
    0x91: "英数", 0xA8: "消音", 0xA9: "音量＋", 0xAA: "音量−",
    0xE0: "Ctrl", 0xE1: "Shift", 0xE2: "Option", 0xE3: "Command",
    0xE4: "右 Ctrl", 0xE5: "右 Shift", 0xE6: "右 Option", 0xE7: "右 Command",
}
KEY_NAMES.update({0x04 + i: chr(ord("A") + i) for i in range(26)})
KEY_NAMES.update({0x1E + i: str((i + 1) % 10) for i in range(10)})
KEY_NAMES.update({0x3A + i: f"F{i + 1}" for i in range(12)})
KEY_NAMES.update({0xD1 + i: label for i, label in enumerate(("左クリック", "右クリック", "中クリック", "戻る", "進む"))})

MOD_BITS = ((1, "Ctrl", "⌃"), (2, "Shift", "⇧"), (4, "Alt", "⌥"), (8, "Win", "⌘"))

USER_ACTIONS = {
    24: ("クリップボード履歴", "⌘⇧V（Mac）／Win+V（Windows）"),
    25: ("置換", "⌥⌘F（Mac）／Ctrl+H（Windows）"),
    28: ("画面ジェスチャー", "押しながらボールを動かして画面を切り替える。"),
    32: ("ウィンドウジェスチャー", "押しながらボールを動かしてウィンドウを操作する。"),
    33: ("タブジェスチャー", "押しながらボールを動かしてタブを切り替える。"),
    34: ("ドラッグ固定", "ドラッグを固定する。"),
    35: ("精密操作", "ポインターを精密に動かす。"),
    36: ("ダブルクリック", "ダブルクリックする。"),
    37: ("ウィンドウ切替", "アプリケーションを切り替える。"),
    38: ("カーソルを遅く", "カーソル速度を下げる。"),
    39: ("カーソルを速く", "カーソル速度を上げる。"),
    40: ("スクロールを少なく", "スクロール量を下げる。"),
    41: ("スクロールを多く", "スクロール量を上げる。"),
    42: ("ジェスチャー感度を下げる", "ジェスチャー感度を下げる。"),
    43: ("ジェスチャー感度を上げる", "ジェスチャー感度を上げる。"),
    44: ("自動マウスを切替", "自動マウスレイヤーを切り替える。"),
    45: ("スクロール反転", "スクロール方向を反転する。"),
    46: ("状態を表示", "現在の状態を表示する。"),
    50: ("Mac／Windows切替", "Mac と Windows の設定を切り替える。"),
    51: ("ズームジェスチャー", "押しながらボールを動かして拡大・縮小する。"),
    52: ("全選択して切り取り", "すべてを選択して切り取る。"),
    53: ("全選択してコピー", "すべてを選択してコピーする。"),
    54: ("行を切り取り", "行頭から行末まで選択して切り取る。"),
    55: ("行をコピー", "行頭から行末まで選択してコピーする。"),
    56: ("段落を選択", "段落を選択する。"),
    57: ("行を選択", "行を選択する。"),
    58: ("単語を選択", "単語を選択する。"),
    59: ("行頭／段落先頭", "1回押しで行頭、すばやく2回押しで段落先頭へ移動する。"),
    60: ("行末／段落末尾", "1回押しで行末、すばやく2回押しで段落末尾へ移動する。"),
    61: ("貼り付け", "貼り付ける。"),
    62: ("最初のタブ", "最初のタブへ移動する。"),
    63: ("最後のタブ", "最後のタブへ移動する。"),
}
MOUSE_ACTIONS = {
    0: ("左クリック", "左クリック。押したままでドラッグする。"),
    1: ("右クリック", "右クリックする。"),
    2: ("中クリック", "中クリックする。"),
    3: ("戻る", "マウスの戻るボタンを押す。"),
    4: ("進む", "マウスの進むボタンを押す。"),
}
OMITTED_USERS = frozenset({0, 1, 2, 3, 5, 6, 7, 47, 48, 49})
MACRO_LABELS = {
    64: "角括弧ペア", 65: "丸括弧ペア", 66: "波括弧ペア", 67: "引用符ペア",
    68: "バッククォートペア", 69: "コードブロック", 70: "全選択してコピー",
    71: "全選択してコピー（Windows）", 72: "00入力", 73: "個人用文字列1",
    74: "個人用文字列2",
}

COMBO_CONTRACT = (
    {
        "id": "COMBO_SETTINGS", "slot": 0, "trigger_codes": (0x00E1, 0x0031, 0, 0),
        "output_code": 0x5286, "label": "設定", "detail": "左 Shift ＋ \\ を押している間、設定レイヤーを使う。",
    },
    {
        "id": "COMBO_PRIVATE_1", "slot": 2, "trigger_codes": (0x0004, 0x0016, 0x0007, 0x0009),
        "output_code": 0x7700, "label": "個人用文字列1",
        "detail": "A ＋ S ＋ D ＋ F を同時押しで登録済みの文字列を入力する。文字列の内容は表示しません。",
    },
    {
        "id": "COMBO_PRIVATE_2", "slot": 3, "trigger_codes": (0x000D, 0x000E, 0x000F, 0x0033),
        "output_code": 0x7701, "label": "個人用文字列2",
        "detail": "J ＋ K ＋ L ＋ ; を同時押しで登録済みの文字列を入力する。文字列の内容は表示しません。",
    },
)

COMBO_POSITIONS = {
    "COMBO_SETTINGS": ("2,0", "6,0"),
    "COMBO_PRIVATE_1": ("1,1", "1,2", "1,3", "1,4"),
    "COMBO_PRIVATE_2": ("5,4", "5,3", "5,2", "5,1"),
}


class GenerationError(ValueError):
    """Raised when the sanitized source contains an action without a port contract."""


def load_snapshot(path: Path) -> dict:
    snapshot = json.loads(path.read_text())
    if set(snapshot) != {"source", "keymap", "combos"}:
        raise GenerationError("Reference must contain only source metadata, keymap, and combos")
    if "macros" in snapshot or "macros" in snapshot.get("source", {}):
        raise GenerationError("Macro payloads must not be stored in the reference")
    if set(snapshot["keymap"]) != {"mac", "win"}:
        raise GenerationError("Reference must contain the actual Mac and Windows banks")
    for profile in ("mac", "win"):
        if tuple(snapshot["keymap"][profile]) != LAYER_ORDER:
            raise GenerationError(f"{profile} bank must contain the six supported layers in order")
        for layer_name, grid in snapshot["keymap"][profile].items():
            if len(grid) != STUDIO_ROWS or any(len(row) != STUDIO_COLS for row in grid):
                raise GenerationError(f"{profile} {layer_name} must be an 8 x 7 Studio grid")
    return snapshot


def _layer_tap_target(target: int, profile: str) -> int:
    names = {1: "edit", 2: "symbol", 3: "num"}
    if target not in names:
        raise GenerationError(f"Layer-Tap target {target} has no Keyball layer mapping")
    return PROFILE_LAYERS[profile][names[target]]


def qmk_code(source_code: int, profile: str) -> int:
    """Translate the CoreX HID/Vial code into the Keyball QMK keycode range."""
    if source_code <= 0x00FF:
        if 0x00D1 <= source_code <= 0x00D5:
            return USER_BASE + source_code - 0x00D1
        return source_code
    if 0x0100 <= source_code <= 0x1FFF:
        return source_code
    if 0x2000 <= source_code <= 0x3FFF:
        return source_code
    if 0x4000 <= source_code <= 0x4FFF:
        source_layer = (source_code >> 8) & 0xF
        target = _layer_tap_target(source_layer, profile)
        return 0x4000 | (target << 8) | (source_code & 0xFF)
    if 0x7700 <= source_code <= 0x77FF:
        slot = source_code & 0xFF
        if slot in (0, 1):
            return USER_BASE + 73 + slot
        if 2 <= slot <= 10:
            return USER_BASE + 64 + slot - 2
        raise GenerationError(f"Macro slot {slot} is outside the approved port")
    if 0x7E00 <= source_code <= 0x7E3F:
        user_id = source_code & 0x3F
        if user_id in USER_ACTIONS:
            return USER_BASE + user_id
        if user_id in OMITTED_USERS:
            return 0
        raise GenerationError(f"CoreX User {user_id} has no Keyball port decision")
    if source_code == 0x7C00:
        return 0  # The Keyball map does not expose a bootloader key.
    raise GenerationError(f"Unreviewed CoreX keycode 0x{source_code:04X}")


def _modifier_names(code: int, profile: str, *, compact: bool = False) -> list[str]:
    mods = (code >> 8) & 0x1F
    names = []
    for bit, win_name, mac_name in MOD_BITS:
        if mods & bit:
            if profile == "mac":
                names.append(mac_name if compact else {"⌃": "Control", "⌥": "Option", "⇧": "Shift", "⌘": "Command"}[mac_name])
            else:
                names.append(win_name)
    return names


def _key_name(code: int) -> str:
    if code in KEY_NAMES:
        return KEY_NAMES[code]
    return "?"


def _key_detail(code: int, profile: str) -> str:
    if code < 0x0100:
        return _key_name(code)
    mods = _modifier_names(code, profile, compact=True)
    return " ＋ ".join(mods + [_key_name(code & 0xFF)])


def describe_code(code: int, profile: str, layer_name: str) -> dict:
    if code == 0:
        return {"code": 0, "label": "—", "detail": "割り当てなし", "kind": "none"}
    if code == 1:
        return {"code": 1, "label": "▽", "detail": "下のレイヤーを使う", "kind": "trns"}
    if 0x4000 <= code <= 0x4FFF:
        target = (code >> 8) & 0xF
        target_name = next(name for name, layer_id in PROFILE_LAYERS[profile].items() if layer_id == target)
        tap = code & 0xFF
        return {
            "code": code, "label": _key_name(tap),
            "detail": f"短押し：{_key_name(tap)} ／ 長押し：{LAYER_INFO[target_name][0]}レイヤー",
            "kind": "dual",
        }
    if 0x2000 <= code <= 0x3FFF:
        tap = code & 0xFF
        hold = " ＋ ".join(_modifier_names(code, profile))
        return {
            "code": code, "label": _key_name(tap),
            "detail": f"短押し：{_key_name(tap)} ／ 長押し：{hold}", "kind": "dual",
        }
    if USER_BASE <= code <= USER_BASE + 74:
        user_id = code - USER_BASE
        if user_id in MOUSE_ACTIONS:
            label, detail = MOUSE_ACTIONS[user_id]
        elif user_id in range(64, 73):
            label = MACRO_LABELS[user_id]
            detail = "CoreX に登録された定型入力。内容はこの表に表示しません。"
        elif user_id in (73, 74):
            label = MACRO_LABELS[user_id]
            detail = "登録済みの個人用文字列。文字列の内容は表示しません。"
        else:
            label, detail = USER_ACTIONS[user_id]
        return {"code": code, "label": label, "detail": detail, "kind": "custom"}
    if 0x0100 <= code <= 0x1FFF:
        return {
            "code": code, "label": _key_detail(code, profile),
            "detail": _key_detail(code, profile), "kind": "key",
        }
    return {"code": code, "label": _key_name(code), "detail": _key_name(code), "kind": "key"}


def _assert_combo_source(snapshot: dict, profile_specs: list[dict]) -> list[dict]:
    raw_combos = snapshot["combos"]
    output = []
    for contract in COMBO_CONTRACT:
        slot = contract["slot"]
        if slot >= len(raw_combos):
            raise GenerationError(f"Missing CoreX combo slot {slot}")
        raw = tuple(int(code, 16) for code in raw_combos[slot])
        expected = contract["trigger_codes"] + (contract["output_code"],)
        if raw != expected:
            raise GenerationError(f"CoreX combo slot {slot} no longer matches the reviewed trigger")
        output.append({
            "id": contract["id"], "keys": list(contract["trigger_codes"]),
            "positions": list(COMBO_POSITIONS[contract["id"]]),
            "label": contract["label"], "detail": contract["detail"],
        })
    for spec in profile_specs:
        base = spec["layers"]["base"]
        for combo in output:
            codes_at_positions = [base[position]["sourceCode"] for position in combo["positions"]]
            if codes_at_positions != combo["keys"][:len(codes_at_positions)]:
                raise GenerationError(f"{combo['id']} triggers do not match the {spec['profile']} base layer")
    return output


def build_profile_spec(snapshot: dict, profile: str) -> dict:
    if profile not in PROFILE_LAYERS:
        raise GenerationError(f"Unknown profile {profile!r}")
    layers = {}
    for layer_name in LAYER_ORDER:
        grid = snapshot["keymap"][profile][layer_name]
        keys = {}
        for key_position, studio in KEYBALL_POSITIONS.items():
            source_code = int(grid[studio[0]][studio[1]], 16)
            code = qmk_code(source_code, profile)
            key = describe_code(code, profile, layer_name)
            key["sourceCode"] = source_code
            keys[f"{key_position[0]},{key_position[1]}"] = key
        layers[layer_name] = keys

    # CoreX's settings bank uses these otherwise empty Keyball positions for portable controls.
    overlays = {
        "0,0": (0x00A9, "音量＋", "音量を上げる。"),
        "1,0": (0x00AA, "音量−", "音量を下げる。"),
        "2,0": (0x00A8, "消音", "音を消す。"),
        "4,1": (0x006B, "F16", "CoreX の F16 操作に使う。"),
    }
    for position, (code, label, detail) in overlays.items():
        layers["settings"][position] = {
            "code": code, "label": label, "detail": detail, "kind": "key", "sourceCode": 0,
        }

    return {
        "profile": profile,
        "layerIds": PROFILE_LAYERS[profile],
        "layerInfo": LAYER_INFO,
        "layers": layers,
        "source": snapshot["source"],
    }


def build_specs(snapshot: dict) -> tuple[dict, dict]:
    mac = build_profile_spec(snapshot, "mac")
    win = build_profile_spec(snapshot, "win")
    combos = _assert_combo_source(snapshot, [mac, win])
    mac["combos"] = combos
    win["combos"] = combos
    return mac, win


def _profile_rows(spec: dict) -> list[str]:
    lines = []
    for layer_name in LAYER_ORDER:
        layer_id = spec["layerIds"][layer_name]
        lines.append(f"  // {spec['profile']} {LAYER_INFO[layer_name][0]}")
        lines.append(f"  [{layer_id}] = {{")
        for row in range(KEYBALL_ROWS):
            cells = []
            labels = []
            for col in range(KEYBALL_COLS):
                key = spec["layers"][layer_name].get(f"{row},{col}")
                cells.append(f"0x{key['code']:04X}" if key else "0x0000")
                labels.append(key["label"] if key else "—")
            lines.append("    {" + ", ".join(cells) + "},  // " + " | ".join(labels))
        lines.append("  },")
    return lines


def render_firmware(mac: dict, win: dict) -> str:
    if mac["profile"] != "mac" or win["profile"] != "win":
        raise GenerationError("Firmware specs must be passed as Mac then Windows")
    if mac["combos"] != win["combos"]:
        raise GenerationError("The Mac and Windows combo triggers must be identical")
    lines = [
        "// Generated from corex-v0.13.0-20261106.json by the CoreX Keyball generator.",
        "// Edit the sanitized source snapshot or generator instead of this file.",
        "",
        "// clang-format off",
        "const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {",
    ]
    lines.extend(_profile_rows(win))
    lines.extend(_profile_rows(mac))
    lines.extend(("};", "", "const uint16_t PROGMEM combo_settings[] = {0x00E1, 0x0031, COMBO_END};"))
    lines.extend((
        "const uint16_t PROGMEM combo_private_1[] = {0x0004, 0x0016, 0x0007, 0x0009, COMBO_END};",
        "const uint16_t PROGMEM combo_private_2[] = {0x000D, 0x000E, 0x000F, 0x0033, COMBO_END};",
        "enum { COMBO_SETTINGS, COMBO_PRIVATE_1, COMBO_PRIVATE_2 };",
        "combo_t key_combos[] = {",
        "  [COMBO_SETTINGS] = COMBO_ACTION(combo_settings),",
        "  [COMBO_PRIVATE_1] = COMBO_ACTION(combo_private_1),",
        "  [COMBO_PRIVATE_2] = COMBO_ACTION(combo_private_2),",
        "};",
        "// clang-format on",
        "",
    ))
    return "\n".join(lines)
