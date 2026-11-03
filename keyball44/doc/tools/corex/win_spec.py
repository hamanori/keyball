"""Derive the Windows layers from the macOS (CoreX) spec, operation by operation.

usage: win_spec.py MAC_SPEC.json OUT_SPEC.json
Codes keep their US meaning; the firmware converts symbols to the JIS keys at send time.
Each changed key records a category: 自動 (clear equivalent), 判断 (needs a decision), 不可 (no equivalent).
"""
import json, sys

mac = json.load(open(sys.argv[1]))
OUT = sys.argv[2]

WIN = {'base': 0, 'edit': 1, 'mouse': 2, 'window': 3, 'num': 4}
MAC_TO_WIN_LAYER = {mac['layerIds'][k]: v for k, v in WIN.items()}
LAYER_INFO = dict(mac['layerInfo'])
LAYER_INFO['window'] = ('ウィンドウ', '英数・\\ を長押し／マウス中に K を長押し。押している間はボールがスクロールになる')

C, S, A, G = 0x01, 0x02, 0x04, 0x08
KC_INT2, KC_LNG1 = 0x88, 0x90
MACRO_URL_COPY = 0x7E4B  # KEYBALL_SAFE_RANGE + 11 (firmware custom keycode)
NAMES = {0x2B: 'Tab', 0x4A: 'Home', 0x4D: 'End', 0x44: 'F11', 0x3D: 'F4', 0x50: '←', 0x4F: '→', 0x52: '↑',
         0x51: '↓', 0x16: 'S', 0x0B: 'H', 0x1C: 'Y', 0x33: ';', 0x1E: '1', 0x1F: '2', 0x20: '3', 0x0F: 'L', 0x06: 'C', 0x27: '0', 0x2D: '-'}


def mk(mods, kc):
    return mods << 8 | kc


def winshort(code):
    mods, kc = code >> 8 & 0x1F, code & 0xFF
    parts = [n for b, n in ((C, 'Ctrl'), (S, 'Shift'), (A, 'Alt'), (G, 'Win')) if mods & b]
    key = NAMES.get(kc) or (chr(65 + kc - 4) if 4 <= kc <= 29 else '0x%02X' % kc)
    return '+'.join(parts + [key])


# Mac code -> (Windows code, label or None, category, rationale)
SHORTCUTS = {
    mk(G, 0x50): (0x4A, None, '自動', '⌘← 行頭 → Home'),
    mk(G, 0x4F): (0x4D, None, '自動', '⌘→ 行末 → End'),
    mk(S | G, 0x21): (mk(S | G, 0x16), None, '自動', '⇧⌘4 範囲スクショ → Win+Shift+S（切り取り領域）'),
    mk(A | G, 0x09): (mk(C, 0x0B), None, '自動', 'VS Code の置換 → Ctrl+H'),
    mk(S | G, 0x1D): (mk(C, 0x1C), None, '自動', 'やり直し → Ctrl+Y'),
    mk(S | G, 0x06): (MACRO_URL_COPY, 'URLコピー', '自動', 'Dia の URL コピー → Ctrl+L、Ctrl+C、Esc を順に送るマクロ'),
    mk(S | A | G, 0x06): (0x0000, '', '不可', 'Markdown リンクのコピーは Chrome/Edge にない → 割り当てなし'),
    mk(S | G, 0x2E): (mk(C, 0x33), None, '自動', '拡大 → Ctrl+;（JIS の「+」キー）'),
    mk(C, 0x52): (mk(G, 0x2B), 'タスクビュー', '自動', 'Mission Control → Win+Tab'),
    mk(C, 0x51): (mk(G, 0x2B), 'タスクビュー', '不可', 'アプリのウィンドウだけの一覧はない → Win+Tab で代用（了承済み）'),
    mk(C, 0x50): (mk(C | G, 0x50), None, '自動', '左のデスクトップ → Ctrl+Win+←'),
    mk(C, 0x4F): (mk(C | G, 0x4F), None, '自動', '右のデスクトップ → Ctrl+Win+→'),
    mk(C, 0x2D): (mk(A, 0x50), None, '自動', 'VS Code の移動履歴を戻る → Alt+←'),
    mk(C | S, 0x2D): (mk(A, 0x4F), None, '自動', 'VS Code の移動履歴を進む → Alt+→'),
    mk(C | G, 0x09): (0x44, None, '自動', 'フルスクリーン → F11'),
    mk(G, 0x14): (mk(A, 0x3D), None, '自動', 'アプリ終了 → Alt+F4'),
    0x68: (mk(G, 0x50), None, '自動', '左に配置（Mac では F13）→ Win+←'),
    0x6D: (mk(G, 0x52), None, '自動', '画面いっぱい（Mac では F18）→ Win+↑'),
    0x6E: (mk(G, 0x4F), None, '自動', '右に配置（Mac では F19）→ Win+→'),
    mk(C | A | G, 0x24): (mk(G, 0x1E), 'Dia', '自動', 'Dia を開く → Win+1（タスクバー1番目）'),
    mk(C | A | G, 0x25): (mk(G, 0x1F), 'Codex', '自動', 'Codex を開く → Win+2'),
    mk(C | A | G, 0x26): (mk(G, 0x20), 'Claude', '自動', 'Claude を開く → Win+3'),
    mk(A, 0xE1): (mk(C, 0xE1), 'Ctrl＋Shift', '自動', '⌥⇧（単語選択）→ Ctrl+Shift。Alt+Shift は入力言語の切り替えになる'),
}
PLAIN = {0xE3: (0xE0, 'Ctrl', '⌘ → Ctrl'), 0xE0: (0xE3, 'Win', '⌃ の位置 → Win'), 0xE2: (0xE2, 'Alt', '⌥ → Alt')}
LABEL_FIX = {'⌘ Cmd': 'Ctrl', '⌥ Opt': 'Alt', '⌃ Ctrl': 'Win', 'Option': 'Alt', 'Option＋Shift': 'Ctrl＋Shift',
             'Dia': 'Dia', 'Codex': 'Codex', 'Claude': 'Claude'}

changes = []


def translate(name, pos, k):
    code, label, detail = k['code'], k['label'], k['detail']
    if code in (0, 1) or 0x7E40 <= code < 0x7E80:
        return dict(k)
    if 0x4000 <= code < 0x5000:
        layer = MAC_TO_WIN_LAYER[code >> 8 & 0xF]
        tap = code & 0xFF
        why = None
        if tap == KC_LNG1:
            tap, why = KC_INT2, 'かな → カタカナひらがなキー（今の Keyball の Windows と同じ）'
        new = 0x4000 | layer << 8 | tap
        if why:
            changes.append((name, pos, label, label, '自動', why))
        return dict(k, code=new)
    if 0x2000 <= code < 0x4000:
        mods = code >> 8 & 0x1F
        if mods & G:
            new = 0x2000 | ((mods & ~G) | C) << 8 | (code & 0xFF)
            changes.append((name, pos, label, label, '自動', '長押しの ⌘ → Ctrl'))
            return dict(k, code=new, detail=detail.replace('⌘ Command', 'Ctrl'))
        return dict(k)
    if code in SHORTCUTS:
        new, newlabel, cat, why = SHORTCUTS[code]
        newlabel = newlabel or label
        changes.append((name, pos, label, newlabel, cat, why))
        if new == 0:
            return dict(k, code=0, label='', detail='割り当てなし', kind='none')
        if new == MACRO_URL_COPY:
            d = 'Ctrl+L → Ctrl+C → Esc（アドレス欄の URL をコピー）'
        elif new < 0x100:
            d = winshort(new) if new not in NAMES else NAMES[new]
        else:
            d = winshort(new)
        return dict(k, code=new, label=newlabel, detail='%s（%s）' % (newlabel, d) if newlabel != d else d)
    if code in PLAIN:
        new, lab, why = PLAIN[code]
        if new != code:
            changes.append((name, pos, label, lab, '自動', why))
        return dict(k, code=new, label=lab, detail=lab)
    if code < 0x100:
        return dict(k)
    mods, kc = code >> 8 & 0x1F, code & 0xFF
    if mods & G and mods & C:
        changes.append((name, pos, label, label, '判断', '⌃⌘ の組み合わせに決まった対応がない'))
        return dict(k)
    if mods & G:
        new = mk((mods & ~G) | C, kc)
        changes.append((name, pos, label, label, '自動', '⌘ → Ctrl'))
        return dict(k, code=new, detail='%s（%s）' % (label, winshort(new)))
    if mods == S:
        return dict(k)  # shifted symbol, converted to JIS by the firmware
    return dict(k, detail='%s（%s）' % (label, winshort(code)))


layers = {}
for name, keys in mac['layers'].items():
    layers[name] = {}
    for pos, k in keys.items():
        nk = translate(name, pos, k)
        nk['label'] = LABEL_FIX.get(nk['label'], nk['label'])
        layers[name][pos] = nk

# Windows macros keep their earlier positions: Tab hold + right pinky column.
layers['edit']['5,0'] = {'code': 0x770B, 'label': 'マクロ11', 'detail': '文字列を入力（中身は secrets.h）', 'kind': 'key'}
layers['edit']['6,0'] = {'code': 0x770C, 'label': 'マクロ12', 'detail': '文字列を入力（中身は secrets.h）', 'kind': 'key'}
changes.append(('edit', '5,0', '', 'マクロ11', '自動', '以前の Windows 配置と同じ位置（Tab 長押し＋右端の中段）'))
changes.append(('edit', '6,0', '', 'マクロ12', '自動', '以前の Windows 配置と同じ位置（Tab 長押し＋右端の下段）'))

spec = dict(mac, layerIds=WIN, layerInfo=LAYER_INFO, layers=layers,
            changes=[dict(zip(('layer', 'pos', 'mac', 'windows', 'category', 'why'), c)) for c in changes])
json.dump(spec, open(OUT, 'w'), ensure_ascii=False, indent=1)
from collections import Counter
print('changes', len(changes), Counter(c[4] for c in changes))
