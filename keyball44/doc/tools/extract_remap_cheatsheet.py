#!/usr/bin/env python3
"""Extract Remap cheatsheet labels into QMK keymap JSON files."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

import pdfplumber


ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / "reference"
OUTPUTS = {
    "windows-japanese": REFERENCE / "keymap-windows-japanese.json",
    "macos-english-us": REFERENCE / "keymap-macos-english-us.json",
}

QMK_ORDER = [
    "L00", "L01", "L02", "L03", "L04", "L05", "R05", "R04", "R03", "R02", "R01", "R00",
    "L10", "L11", "L12", "L13", "L14", "L15", "R15", "R14", "R13", "R12", "R11", "R10",
    "L20", "L21", "L22", "L23", "L24", "L25", "R25", "R24", "R23", "R22", "R21", "R20",
    "L31", "L32", "L33", "L34", "L35", "R35", "R34", "R31",
]

CUSTOM_KEYCODES = [
    {"key": "KC_MY_BTN1", "label": "User 0"},
    {"key": "KC_MY_BTN2", "label": "User 1"},
    {"key": "KC_MY_BTN4", "label": "User 3"},
    {"key": "KC_MY_BTN5", "label": "User 4"},
    {"key": "KC_MY_SCR", "label": "User 5"},
    {"key": "KC_SCR_SPD_INC", "label": "User 8"},
    {"key": "KC_SCR_SPD_DEC", "label": "User 9"},
    {"key": "KC_SCR_RST", "label": "User 10"},
]


def position_template() -> list[dict[str, object]]:
    positions: list[dict[str, object]] = []

    left_x = [68.0, 124.0, 180.0, 236.0, 292.0, 348.0]
    left_y = [
        [125.6, 181.6, 237.6],
        [125.6, 181.6, 237.6],
        [106.0, 162.0, 218.0],
        [92.0, 148.0, 204.0],
        [99.0, 155.0, 211.0],
        [106.0, 162.0, 218.0],
    ]
    right_x = [656.0, 712.0, 768.0, 824.0, 880.0, 936.0]
    right_y = [
        [106.0, 162.0, 218.0],
        [99.0, 155.0, 211.0],
        [92.0, 148.0, 204.0],
        [106.0, 162.0, 218.0],
        [125.6, 181.6, 237.6],
        [125.6, 181.6, 237.6],
    ]

    for column, x in enumerate(left_x):
        for row in range(3):
            positions.append(
                {
                    "position": f"L{row}{column}",
                    "side": "left",
                    "row": row,
                    "column": column,
                    "x": x,
                    "y": left_y[column][row],
                }
            )

    for column, x in enumerate(right_x):
        for row in range(3):
            positions.append(
                {
                    "position": f"R{row}{5 - column}",
                    "side": "right",
                    "row": row,
                    "column": column,
                    "x": x,
                    "y": right_y[column][row],
                }
            )

    positions.extend(
        [
            {"position": "L31", "side": "left", "row": 3, "column": 0, "x": 180.0, "y": 274.0},
            {"position": "L32", "side": "left", "row": 3, "column": 1, "x": 236.0, "y": 260.0},
            {"position": "L33", "side": "left", "row": 3, "column": 2, "x": 308.8, "y": 288.0},
            {"position": "L34", "side": "left", "row": 3, "column": 3, "x": 372.0, "y": 293.3},
            {"position": "L35", "side": "left", "row": 3, "column": 4, "x": 432.5, "y": 310.4},
            {"position": "R35", "side": "right", "row": 3, "column": 0, "x": 571.5, "y": 310.8},
            {"position": "R34", "side": "right", "row": 3, "column": 1, "x": 632.2, "y": 294.4},
            {"position": "R31", "side": "right", "row": 3, "column": 2, "x": 880.0, "y": 293.6},
        ]
    )
    return positions


PROFILES = {
    "windows-japanese": REFERENCE / "remap-windows-japanese.pdf",
    "macos-english-us": REFERENCE / "remap-mac-english-us.pdf",
}


def key_curves(page: pdfplumber.page.Page, top: float, bottom: float) -> list[dict]:
    curves = []
    for curve in page.curves:
        if not curve.get("stroke") or curve.get("fill"):
            continue
        if curve["top"] <= top or curve["bottom"] >= bottom:
            continue
        if not (50 <= curve["width"] < 70 and 50 <= curve["height"] < 70):
            continue
        curves.append(curve)
    return curves


def label_for_words(words: list[dict]) -> str:
    selected = sorted(words, key=lambda word: (word["top"], word["x0"]))
    tokens = [word["text"] for word in selected]

    if tokens in (["B", "S"], ["S", "B"]):
        return "BS"
    if tokens == ["か", "な"]:
        return "かな"
    if tokens == ["l", "e", "D"]:
        return "Del"
    return " ".join(tokens)


def nearest_position(cx: float, cy: float, positions: list[dict[str, object]]) -> dict[str, object]:
    position = min(
        positions,
        key=lambda item: (float(item["x"]) - cx) ** 2 + (float(item["y"]) - cy) ** 2,
    )
    distance = ((float(position["x"]) - cx) ** 2 + (float(position["y"]) - cy) ** 2) ** 0.5
    if distance > 5:
        raise ValueError(f"Could not map key at ({cx:.1f}, {cy:.1f}); nearest distance is {distance:.1f}")
    return position


def remap_label_to_qmk(label: str) -> str:
    """Convert the common labels in a Remap cheatsheet to QMK syntax."""
    basic = {
        "▽": "_______",
        "Esc": "KC_ESC",
        "Tab": "KC_TAB",
        "BS": "KC_BSPC",
        "Del": "KC_DEL",
        "Enter": "KC_ENT",
        "Print Screen": "KC_PSCR",
        "Home": "KC_HOME",
        "End": "KC_END",
        "Page Up": "KC_PGUP",
        "Page Down": "KC_PGDN",
        "↑": "KC_UP",
        "↓": "KC_DOWN",
        "←": "KC_LEFT",
        "→": "KC_RGHT",
        "Mute": "KC_MUTE",
        "Space": "KC_SPC",
        "英数": "KC_LANG2",
        "かな": "KC_LANG1",
        "Lang 1": "KC_LANG1",
        "Lang 2": "KC_LANG2",
        "*Ctrl": "KC_LCTL",
        "Ctrl*": "KC_LCTL",
        "*Alt": "KC_LALT",
        "*Shift": "KC_LSFT",
        "*Win": "KC_LGUI",
    }
    if label in basic:
        return basic[label]
    if label.startswith("F") and label[1:].isdigit():
        return f"KC_{label}"
    if len(label) == 1 and label.isalpha():
        return f"KC_{label.upper()}"
    if label in {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"}:
        return f"KC_{label}"

    punctuation = {
        "= -": "KC_MINS", "_ -": "KC_MINS", "+ ;": "KC_SCLN", ": ;": "KC_SCLN",
        "(*S+7) '": "KC_QUOT", '" \'': "KC_QUOT", "? /": "KC_SLSH", "> .": "KC_DOT",
        "< ,": "KC_COMM", "¥ Layer(3)": "LT(3,KC_INT1)", "\\ Layer(3)": "LT(3,KC_BSLS)",
        "(*S+¥) _": "KC_INT1", "(*S+¥) |": "KC_INT1", "~ ^": "KC_GRV", "~ `": "KC_GRV",
        "+ =": "KC_EQL", "(*S+=) +": "KC_EQL", "(*S+-) =": "KC_MINS", "(*S+-) _": "KC_MINS",
        "{ [": "KC_LBRC", "} ]": "KC_RBRC", "(*S+[) {": "KC_LBRC", "(*S+]) }": "KC_RBRC",
        "(*S+;) +": "KC_SCLN", "(*S+.) >": "KC_DOT", "(*S+,) <": "KC_COMM", "(*S+/) ?": "KC_SLSH",
        "(*S+@) `": "KC_GRV", "(*S+^) ~": "KC_GRV",
    }
    if label in punctuation:
        return punctuation[label]

    if label.startswith("Tab Layer("):
        return "LT(1,KC_TAB)"
    if label.startswith("Space Layer("):
        return "LT(4,KC_SPC)"
    if label.startswith("Space *Shift"):
        return "MT(MOD_LSFT,KC_SPC)"
    if label.startswith("英数 Layer(") or label.startswith("Lang 2 Layer("):
        return "LT(3,KC_LANG2)"
    if label.startswith("かな Layer(") or label.startswith("か な Layer(") or label.startswith("Lang 1 Layer("):
        return "LT(1,KC_LANG1)"
    if label.startswith("Enter Layer("):
        return "LT(2,KC_ENT)"

    if label.startswith("User "):
        user_keycodes = {
            "User 0": "KC_MY_BTN1", "User 1": "KC_MY_BTN2", "User 3": "KC_MY_BTN4",
            "User 4": "KC_MY_BTN5", "User 8": "KC_SCR_SPD_INC", "User 9": "KC_SCR_SPD_DEC",
            "User 10": "KC_SCR_RST",
        }
        return user_keycodes.get(label, "KC_NO")
    if label == "Kb 2":
        return "CPI_I100"
    if label == "Kb 3":
        return "CPI_D100"
    if label.startswith("Macro ") or label.startswith("Any "):
        return "KC_NO"

    if label.startswith("*Win/Cmd "):
        return f"LGUI({remap_label_to_qmk(label.removeprefix('*Win/Cmd ').strip())})"
    if label.startswith("*Ctrl "):
        return f"LCTL({remap_label_to_qmk(label.removeprefix('*Ctrl ').strip())})"
    if label.startswith("*Alt "):
        return f"LALT({remap_label_to_qmk(label.removeprefix('*Alt ').strip())})"
    if label.startswith("*Shift "):
        return f"LSFT({remap_label_to_qmk(label.removeprefix('*Shift ').strip())})"
    if label.startswith("*C+S "):
        return f"LSFT(LCTL({remap_label_to_qmk(label.removeprefix('*C+S ').strip())}))"
    if label.startswith("*C+A "):
        return f"LALT(LCTL({remap_label_to_qmk(label.removeprefix('*C+A ').strip())}))"
    if label.startswith("*C+W "):
        return f"LGUI(LCTL({remap_label_to_qmk(label.removeprefix('*C+W ').strip())}))"
    if label.startswith("*A+W "):
        return f"LGUI(LALT({remap_label_to_qmk(label.removeprefix('*A+W ').strip())}))"
    if label.startswith("*Win/Cmd"):
        return "KC_LGUI"

    shifted = re.fullmatch(r"\(\*S\+(.+)\)\s+.+", label)
    if shifted:
        key = shifted.group(1)
        if key.isdigit():
            return f"S(KC_{key})"
        if key == "=":
            return "S(KC_EQL)"
        if key == "`":
            return "S(KC_GRV)"
        if key == "-":
            return "S(KC_MINS)"
        if key in {":", ";"}:
            return "S(KC_SCLN)"
        if key in {"¥", "/", ".", ","}:
            return "KC_NO"

    if label == "*S+W 4":
        return "S(KC_W)"
    if label == "* :":
        return "S(KC_SCLN)"
    if label == "` @":
        return "KC_2"
    if label.startswith("t l A "):
        return "LALT(KC_BSPC)"
    digit_pair = re.fullmatch(r".+\s([0-9])", label)
    if digit_pair:
        return f"KC_{digit_pair.group(1)}"

    return "KC_NO"


def extract_profile(profile: str, pdf_path: Path, positions: list[dict[str, object]]) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    with pdfplumber.open(pdf_path) as pdf:
        page = pdf.pages[0]
        frames = [
            curve
            for curve in page.curves
            if curve.get("stroke")
            and not curve.get("fill")
            and curve["width"] > 900
            and curve["height"] > 290
        ]
        frames.sort(key=lambda curve: curve["top"])
        if len(frames) != 7:
            raise ValueError(f"{pdf_path.name}: expected 7 layer frames, found {len(frames)}")

        words = page.extract_words(x_tolerance=1, y_tolerance=2, keep_blank_chars=False)
        first_frame_top = frames[0]["top"]
        for layer, frame in enumerate(frames):
            curves = key_curves(page, frame["top"], frame["bottom"])
            if len(curves) != 44:
                raise ValueError(f"{pdf_path.name}: layer {layer} expected 44 keys, found {len(curves)}")

            word_groups: list[list[dict]] = [[] for _ in curves]
            for word in words:
                wx = (word["x0"] + word["x1"]) / 2
                wy = (word["top"] + word["bottom"]) / 2
                if not (frame["top"] < wy < frame["bottom"]):
                    continue
                nearest_curve = min(
                    range(len(curves)),
                    key=lambda index: (
                        ((curves[index]["x0"] + curves[index]["x1"]) / 2 - wx) ** 2
                        + ((curves[index]["top"] + curves[index]["bottom"]) / 2 - wy) ** 2
                    ),
                )
                word_groups[nearest_curve].append(word)

            used: set[str] = set()
            y_offset = frame["top"] - first_frame_top
            for curve_index, curve in enumerate(curves):
                cx = (curve["x0"] + curve["x1"]) / 2
                cy = (curve["top"] + curve["bottom"]) / 2
                position_candidates = [
                    {**position, "y": float(position["y"]) + y_offset}
                    for position in positions
                ]
                position = nearest_position(cx, cy, position_candidates)
                position_name = str(position["position"])
                if position_name in used:
                    raise ValueError(f"{pdf_path.name}: duplicate position {position_name} in layer {layer}")
                used.add(position_name)
                raw_label = label_for_words(word_groups[curve_index])
                rows.append({
                    "profile": profile,
                    "layer": layer,
                    "position": position_name,
                    "side": position["side"],
                    "row": position["row"],
                    "column": position["column"],
                    "x_center_pt": f"{cx:.1f}",
                    "y_center_pt": f"{cy:.1f}",
                    "raw_label": raw_label,
                    "qmk_keycode": remap_label_to_qmk(raw_label),
                    "source_pdf": pdf_path.name,
                })
            if len(used) != 44:
                raise ValueError(f"{pdf_path.name}: layer {layer} mapped {len(used)} positions")
    return rows


def main() -> int:
    positions = position_template()
    rows: list[dict[str, object]] = []
    for profile, path in PROFILES.items():
        rows.extend(extract_profile(profile, path, positions))

    for profile, output in OUTPUTS.items():
        profile_rows = [row for row in rows if row["profile"] == profile]
        layers = []
        for layer in range(7):
            layer_rows = {row["position"]: row for row in profile_rows if row["layer"] == layer}
            layers.append([layer_rows[position]["qmk_keycode"] for position in QMK_ORDER])

        # The PDFs are historical snapshots. The mouse-session contract uses
        # User 5 at the Layer 6 K position for hold-scroll.
        layers[6][QMK_ORDER.index("R13")] = "KC_MY_SCR"

        data = {
            "keyboard": "keyball/keyball44",
            "keymap": profile.replace("-", "_"),
            "version": 1,
            "layout": "LAYOUT_right_ball",
            "layers": layers,
            "keycodes": CUSTOM_KEYCODES,
            "notes": (
                "Generated from the Remap cheatsheet snapshot. "
                "Layer 6 K is intentionally changed from Kb 2 to User 5 for "
                "hold-scroll in the mouse-session specification. "
                "Macro and Any entries are intentionally stored as KC_NO for now. "
                f"Source: {profile_rows[0]['source_pdf']}"
            ),
        }
        output.write_text(json.dumps(data, ensure_ascii=False, indent=4) + "\n", encoding="utf-8")
        print(f"wrote {len(profile_rows)} source rows to {output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
