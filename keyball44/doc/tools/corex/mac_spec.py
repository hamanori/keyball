"""CoreX (Mac) -> Keyball44 static Mac layers: single source for firmware and cheat sheet.

usage: mac_spec.py COREX_BACKUP.json COREX_REPORT.json OUT_SPEC.json
"""
import json, sys

COREX = json.load(open(sys.argv[1]))['configuration']
NOTES = json.load(open(sys.argv[2]))['layerNotes']
OUT = sys.argv[3]

# Firmware layer ids for the Mac profile (Windows uses 0-4, derived by win_spec.py).
MAC = {'base': 5, 'edit': 6, 'mouse': 7, 'window': 8, 'num': 9}
COREX_TO_MAC = {0: 'base', 1: 'edit', 2: 'mouse', 3: 'window', 4: 'num'}
LAYER_INFO = {
    'base': ('通常', '文字入力と、下に書いたコンボ'),
    'edit': ('編集・移動', 'Tab を長押し'),
    'mouse': ('マウス', 'ボールを動かすと自動で入る／かなを長押し'),
    'window': ('ウィンドウ', '英数・\\ を長押し／マウス中に K を長押し。押している間はボールがスクロールになる'),
    'num': ('数字・記号', 'Enter を長押し'),
}

# Keyball (row, col) -> CoreX Studio id "r-c".
POS = {}
for c in range(6):
    POS[(0, c)], POS[(1, c)], POS[(2, c)] = '4-%d' % c, '5-%d' % c, '6-%d' % c
RIGHT = [['0-0', '0-1', '0-2', '0-3', '0-4', '3-4'],
         ['1-0', '1-1', '1-2', '1-3', '1-4', '1-5'],
         ['2-0', '2-1', '2-2', '2-3', '2-4', '2-5']]
for r in range(3):
    for c in range(6):
        POS[(4 + r, c)] = RIGHT[r][c]
POS.update({(3, 3): '7-3', (3, 4): '7-4', (3, 5): '7-5', (7, 5): '3-5', (7, 4): '3-3'})
EXTRA = [(3, 1), (3, 2), (7, 1)]  # Keyball-only thumb positions
USED = sorted(POS) + EXTRA

NO, TRNS = 0x0000, 0x0001
LT = lambda layer, kc: 0x4000 | (layer & 0xF) << 8 | kc
MY_BTN1, MY_BTN2, MY_BTN3, MY_BTN4, MY_BTN5, MY_SCR = range(0x7E40, 0x7E46)

BASIC = {0x28: 'Enter', 0x29: 'Esc', 0x2A: '⌫', 0x2B: 'Tab', 0x2C: 'Space', 0x2D: '-', 0x2E: '=', 0x2F: '[',
         0x30: ']', 0x31: '\\', 0x33: ';', 0x34: "'", 0x35: '`', 0x36: ',', 0x37: '.', 0x38: '/', 0x4C: 'Delete',
         0x4B: 'Page Up', 0x4E: 'Page Down', 0x4F: '→', 0x50: '←', 0x51: '↓', 0x52: '↑', 0x90: 'かな', 0x91: '英数',
         0xA8: '消音', 0xA9: '音量 ＋', 0xAA: '音量 −', 0xE0: '⌃ Control', 0xE1: '⇧ Shift', 0xE2: '⌥ Option',
         0xE3: '⌘ Command', 0xD1: '左クリック', 0xD2: '右クリック', 0xD3: '中クリック', 0xD4: '戻る', 0xD5: '進む'}
for i in range(26):
    BASIC[4 + i] = chr(65 + i)
for i in range(9):
    BASIC[0x1E + i] = str(i + 1)
BASIC[0x27] = '0'
for i in range(12):
    BASIC[0x3A + i] = 'F%d' % (i + 1)
for i in range(12):
    BASIC[0x68 + i] = 'F%d' % (i + 13)
US_SHIFT = dict(zip('1234567890-=[]\\;\',./`', '!@#$%^&*()_+{}|:"<>?~'))
MODSYM = [(0x01, '⌃'), (0x04, '⌥'), (0x02, '⇧'), (0x08, '⌘')]


def keyname(kc):
    return BASIC.get(kc, '0x%02X' % kc)


def shortcut(code):
    """Exact keys sent, Mac notation."""
    if code < 0x100:
        return keyname(code)
    mods, kc = code >> 8 & 0x1F, code & 0xFF
    return ''.join(s for b, s in MODSYM if mods & b) + keyname(kc)


def symbol_of(code):
    """Character a Shift+key code types on a US layout, else None."""
    if code >> 8 == 0x02 and keyname(code & 0xFF) in US_SHIFT:
        return US_SHIFT[keyname(code & 0xFF)]
    return None


def corex_code(layer, cid):
    r, c = map(int, cid.split('-'))
    return COREX['keymap'][layer * 56 + r * 7 + c]


def retarget(code):
    """Re-encode CoreX layer targets onto the Mac layer ids."""
    if 0x4000 <= code < 0x5000:
        return LT(MAC[COREX_TO_MAC[code >> 8 & 0xF]], code & 0xFF)
    return code


def note(layer, cid):
    e = NOTES.get(str(layer), {}).get('entries', {}).get('key-' + cid)
    return e['label'].replace('\n', '') if e and e.get('label') else None


def key(code, label, detail, kind='key'):
    return {'code': code, 'label': label, 'detail': detail, 'kind': kind}


def describe(layer, cid, code):
    if code == TRNS:
        return key(code, '', '下のレイヤーと同じ', 'trns')
    if code == NO:
        return key(code, '', '割り当てなし', 'none')
    if 0x4000 <= code < 0x5000:
        target = LAYER_INFO[COREX_TO_MAC[code >> 8 & 0xF]][0]
        tap = keyname(code & 0xFF)
        return key(code, tap, '短押し：%s ／ 長押し：%sレイヤー' % (tap, target), 'dual')
    if 0x2000 <= code < 0x4000:
        mods = code >> 8 & 0x1F
        hold = ''.join(s for b, s in MODSYM if mods & b)
        hold = {'⌘': '⌘ Command', '⇧': '⇧ Shift'}.get(hold, hold)
        tap = keyname(code & 0xFF)
        return key(code, tap, '短押し：%s ／ 長押し：%s' % (tap, hold), 'dual')
    sym = symbol_of(code)
    if sym:
        return key(code, sym, '%s を入力（%s）' % (sym, shortcut(code)))
    label = note(layer, cid) if layer != 4 else None  # L4 notes in the CoreX report are stale
    if layer == 4 and cid in ('7-3', '7-4', '7-5'):
        label = note(layer, cid)
    if code < 0x100:
        return key(code, label or keyname(code), keyname(code) if not label or label == keyname(code) else '%s（%s）' % (label, keyname(code)))
    return key(code, label or shortcut(code), shortcut(code))


layers = {}
for cl, name in COREX_TO_MAC.items():
    keys = {}
    for (r, c), cid in POS.items():
        code = corex_code(cl, cid)
        k = describe(cl, cid, code)
        k['code'] = retarget(code)
        keys['%d,%d' % (r, c)] = k
    layers[name] = keys

# --- Keyball-specific changes (each listed in the proposal) ---
b, e, m, w, n = (layers[x] for x in ('base', 'edit', 'mouse', 'window', 'num'))
# TD(0) 英数/L3 (250 ms) -> Layer Tap with a 250 ms per-key term in firmware.
b['3,3'] = key(LT(MAC['window'], 0x91), '英数', '短押し：英数 ／ 長押し：ウィンドウレイヤー（判定 250ms）', 'dual')
b['3,1'] = key(0xE2, '⌥ Opt', '⌥ Option')
b['3,2'] = key(0xE3, '⌘ Cmd', '⌘ Command')
b['7,1'] = key(0xE0, '⌃ Ctrl', '⌃ Control（CoreX の Spotlight の位置。Spotlight は ⌘＋Space）')
e['3,1'], e['3,2'], e['7,1'] = key(0xE2, '⌥ Opt', '⌥ Option'), key(0xE3, '⌘ Cmd', '⌘ Command'), key(0xE0, '⌃ Ctrl', '⌃ Control')
for L in (m, w):
    for p in ('3,1', '3,2', '7,1'):
        L[p] = key(TRNS, '', '下のレイヤーと同じ', 'trns')
for p in ('3,1', '3,2', '7,1'):
    n[p] = key(NO, '', '割り当てなし', 'none')
# Mouse layer: session-aware buttons; the gesture key becomes hold-to-scroll.
m['5,4'] = key(MY_BTN1, '左クリック', '左クリック（押したままでドラッグ）')
m['5,2'] = key(MY_BTN2, '右クリック', '右クリック')
m['6,4'] = key(MY_BTN4, '戻る', 'マウスの戻るボタン')
m['6,2'] = key(MY_BTN5, '進む', 'マウスの進むボタン')
m['5,3'] = key(LT(MAC['window'], 0xD3), '中クリック', '短押し：中クリック ／ 長押し：ウィンドウレイヤー', 'dual')
m['4,3'] = key(MY_SCR, 'スクロール', '押している間、ボールでスクロール（CoreX のジェスチャーキーの位置）')
# Volume dial -> empty left pinky column of the number layer.
n['0,0'] = key(0xA9, '音量 ＋', '音量を上げる（CoreX の左ダイヤル）')
n['1,0'] = key(0xAA, '音量 −', '音量を下げる（CoreX の左ダイヤル）')
n['2,0'] = key(0xA8, '消音', '消音（CoreX の左ダイヤル押し込み）')

# Combos: CoreX symbol combos, mapped by keycode (same keys exist on the Keyball base layer).
base_by_code = {v['code']: p for p, v in b.items()}
combos = []
for a, bb, _, _, out in COREX['combos']:
    if not a:
        continue
    if a not in base_by_code or bb not in base_by_code or out >= 0x5000:
        combos.append({'keys': [a, bb], 'output': out, 'ported': False})
        continue
    sym = symbol_of(out) or keyname(out)
    combos.append({'keys': [a, bb], 'pos': [base_by_code[a], base_by_code[bb]], 'output': out, 'symbol': sym, 'ported': True})

# Shift + \\ switches between the macOS and Windows profiles (firmware toggle combo).
toggle = {'pos': ['2,0', '6,0'], 'symbol': '⇄', 'label': 'Mac と Windows を切り替え'}
json.dump({'layerIds': MAC, 'layerInfo': LAYER_INFO, 'used': ['%d,%d' % p for p in USED],
           'layers': layers, 'combos': combos, 'toggle': toggle, 'comboTermMs': 30, 'tappingTermMs': 200},
          open(OUT, 'w'), ensure_ascii=False, indent=1)
print('combos ported', sum(c['ported'] for c in combos), 'skipped', [c for c in combos if not c['ported']])
