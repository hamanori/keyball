"""Render the two Keyball44 profiles as one responsive Japanese cheat sheet."""

import html
import json
from pathlib import Path
import sys

from corex_generation import LAYER_ORDER


def grid_position(position: str) -> tuple[int, int]:
    row, col = map(int, position.split(","))
    if row < 3:
        return row, col
    if 3 < row < 7:
        return row - 4, 13 - col
    thumb_col = {
        (3, 1): 2, (3, 2): 3, (3, 3): 4, (3, 4): 5,
        (3, 5): 6, (7, 5): 7, (7, 4): 8, (7, 1): 12,
    }[(row, col)]
    return 3, thumb_col


COLORS = ("#c2410c", "#0f766e", "#7c3aed")


def key_html(position: str, key: dict, badges: list[tuple[int, str, str]]) -> str:
    row, col = grid_position(position)
    col += col >= 7
    classes = ["key", key["kind"]]
    if row == 3:
        classes.append("thumb")
    label = html.escape(key["label"])
    detail = html.escape(key["detail"])
    size = " long" if len(key["label"]) > 5 else ""
    badge_html = "".join(
        f'<span class="badge" style="--c:{color}" data-combo="{index}">{html.escape(text)}</span>'
        for index, text, color in badges
    )
    return (
        f'<div class="{" ".join(classes)}" style="grid-row:{row + 1};grid-column:{col + 1}" '
        f'data-pos="{position}" data-detail="{detail}" tabindex="0" aria-label="{label}">'
        f'<span class="lbl{size}">{label}</span>{badge_html}</div>'
    )


def layer_html(spec: dict, profile: str, layer_name: str) -> str:
    title, how = spec["layerInfo"][layer_name]
    keys = spec["layers"][layer_name]
    items = []
    badges: dict[str, list[tuple[int, str, str]]] = {}
    legend = []
    if layer_name == "base":
        for index, combo in enumerate(spec["combos"]):
            color = COLORS[index % len(COLORS)]
            positions = combo["positions"]
            for position in positions:
                badges.setdefault(position, []).append((index, str(index + 1), color))
            names = " ＋ ".join(keys[position]["label"] for position in positions)
            legend.append(
                f'<li data-combo="{index}" tabindex="0"><b style="color:{color}">'
                f'{index + 1}. {html.escape(combo["label"])}</b> — {names}。'
                f'{html.escape(combo["detail"])}</li>'
            )
    for position, key in keys.items():
        items.append(key_html(position, key, badges.get(position, [])))
    out = [
        f'<section class="layer" id="{profile}-{layer_name}"><h2>{html.escape(title)}'
        f'<small>{html.escape(how)}</small></h2>',
        f'<div class="board-wrap"><div class="board">{"".join(items)}</div></div>',
    ]
    if legend:
        out.append(
            '<div class="legend"><h3>コンボ</h3><p>該当するキーを同時に押します。設定コンボは押している間だけ有効です。</p>'
            f'<ol>{"".join(legend)}</ol></div>'
        )
    out.append("</section>")
    return "".join(out)


CSS = """
:root{--bg:#f7f7f5;--fg:#1a1a1a;--muted:#666;--key:#fff;--keyb:#d4d4d0;--thumb:#eef6f5;--dual:#fff8eb;--trns:#f1f1ef;--accent:#0f766e;--combo:#c2410c;--card:#fff}
@media(prefers-color-scheme:dark){:root:not([data-theme=light]){--bg:#161616;--fg:#eee;--muted:#aaa;--key:#262626;--keyb:#3a3a3a;--thumb:#1f2e2c;--dual:#2e2717;--trns:#1d1d1d;--accent:#5eead4;--combo:#fb923c;--card:#1e1e1e}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font-family:"Noto Sans JP","Hiragino Sans",system-ui,sans-serif;line-height:1.5}
main{max-width:1180px;margin:0 auto;padding:16px}h1{font-size:1.5rem;margin:.5rem 0}h2{font-size:1.15rem;margin:1.6rem 0 .5rem}h2 small{font-weight:400;color:var(--muted);font-size:.85rem;margin-left:.75rem}
.lead,.muted{color:var(--muted)}nav{display:flex;gap:.5rem;flex-wrap:wrap;margin:1rem 0}nav a{color:var(--accent);text-decoration:none;border:1px solid var(--keyb);border-radius:999px;padding:.2rem .8rem;font-size:.9rem}
.board-wrap{overflow-x:auto;padding:14px 0 18px}.board{--u:clamp(38px,5.2vw,66px);display:grid;grid-template-columns:repeat(7,var(--u)) 18px repeat(7,var(--u));grid-template-rows:repeat(4,var(--u));gap:6px;width:max-content;position:relative}
.key{position:relative;background:var(--key);border:1px solid var(--keyb);border-radius:10px;display:flex;align-items:center;justify-content:center;text-align:center;padding:3px;cursor:default;outline:none}.key.thumb{background:var(--thumb)}.key.dual{background:var(--dual)}.key.trns,.key.none{background:var(--trns);border-style:dashed}
.key:hover,.key:focus,.key.hl{border-color:var(--accent);box-shadow:0 0 0 2px var(--accent)}.lbl{font-weight:700;font-size:clamp(.68rem,1.2vw,.95rem);line-height:1.2;word-break:keep-all;overflow-wrap:anywhere}.lbl.long{font-size:clamp(.58rem,1vw,.8rem)}
.badge{position:absolute;bottom:3px;right:3px;background:var(--c);color:#fff;font-size:.72rem;font-weight:700;border-radius:999px;min-width:18px;height:18px;line-height:18px;padding:0 4px;text-align:center}.badge+.badge{right:24px}
.legend ol{padding-left:1.4rem}.legend li{font-size:.92rem;cursor:default;margin:.35rem 0}.legend p,.foot{color:var(--muted);font-size:.85rem}
#tip{position:fixed;z-index:10;max-width:300px;background:var(--fg);color:var(--bg);font-size:.85rem;padding:.45rem .65rem;border-radius:8px;pointer-events:none;display:none}
.switch{display:inline-flex;border:1px solid var(--keyb);border-radius:999px;padding:3px;gap:3px;background:var(--card)}.switch button{border:0;background:none;color:var(--fg);font:inherit;font-weight:700;padding:.3rem 1.1rem;border-radius:999px;cursor:pointer}.switch button[aria-selected=true]{background:var(--accent);color:var(--bg)}
.os{display:none}.os.on{display:block}.notes{color:var(--muted);font-size:.92rem;padding-left:1.2rem;margin:.8rem 0}@media print{nav,.switch{display:none}.layer{break-inside:avoid}}
"""

JS = """
const tip=document.getElementById('tip');
function show(el,x,y){tip.textContent=el.dataset.detail;tip.style.display='block';const w=tip.offsetWidth,h=tip.offsetHeight;tip.style.left=Math.min(innerWidth-w-8,Math.max(8,x-w/2))+'px';tip.style.top=(y-h-12<8?y+16:y-h-12)+'px'}
function hide(){tip.style.display='none'}
document.querySelectorAll('.key').forEach(k=>{k.addEventListener('mouseenter',e=>{const r=k.getBoundingClientRect();show(k,r.left+r.width/2,r.top)});k.addEventListener('mouseleave',hide);k.addEventListener('focus',()=>{const r=k.getBoundingClientRect();show(k,r.left+r.width/2,r.top)});k.addEventListener('blur',hide)});
const comboKeys=%s;
function highlight(root,id,on){root.querySelectorAll('[data-combo="'+id+'"]').forEach(e=>{e.classList.toggle('hl',on);const k=e.closest('.key');if(k)k.classList.toggle('hl',on)});comboKeys[id].forEach(p=>{const k=root.querySelector('.layer[id$="-base"] .key[data-pos="'+p+'"]');if(k)k.classList.toggle('hl',on)})}
document.querySelectorAll('.legend li,.badge').forEach(e=>{const id=Number(e.dataset.combo),root=e.closest('.os');e.addEventListener('mouseenter',()=>highlight(root,id,true));e.addEventListener('mouseleave',()=>highlight(root,id,false));e.addEventListener('focus',()=>highlight(root,id,true));e.addEventListener('blur',()=>highlight(root,id,false))});
function pick(os){document.querySelectorAll('.os').forEach(e=>e.classList.toggle('on',e.dataset.os===os));document.querySelectorAll('.switch button').forEach(e=>e.setAttribute('aria-selected',e.dataset.os===os));try{localStorage.setItem('keyball-os',os)}catch(e){}}
document.querySelectorAll('.switch button').forEach(e=>e.addEventListener('click',()=>pick(e.dataset.os)));let first=(location.hash.match(/^#(mac|win)/)||[])[1];try{first=first||localStorage.getItem('keyball-os')}catch(e){}pick(['mac','win'].includes(first)?first:(/Windows/.test(navigator.userAgent)?'win':'mac'));
"""

NOTES = {
    "mac": (
        "US 配列で使います。",
        "画面・ウィンドウ・タブ・ズームのジェスチャーキーを押しながら、ボールを上下左右へ動かします。ダブルクリックは専用キーです。",
        "個人用文字列を送るコンボがあります。文字列の内容は表示しません。",
    ),
    "win": (
        "Windows のキー配置とショートカットに合わせた設定です。",
        "画面・ウィンドウ・タブ・ズームのジェスチャーキーを押しながら、ボールを上下左右へ動かします。ダブルクリックは専用キーです。",
        "個人用文字列を送るコンボがあります。文字列の内容は表示しません。",
    ),
}


def render(mac: dict, win: dict) -> str:
    combo_keys = [combo["positions"] for combo in mac["combos"]]
    body = [
        "<main><h1>Keyball44 早見表</h1>",
        '<p class="lead">CoreX v0.13.0 の Mac／Windows 操作を、Keyball44 のキーに割り当てた早見表です。キーをタップするかカーソルを合わせると、操作内容を確認できます。狭い画面では盤面を左右にスクロールできます。</p>',
        '<div class="switch" role="tablist"><button role="tab" data-os="mac">Mac</button><button role="tab" data-os="win">Windows</button></div>',
    ]
    for profile, spec in (("mac", mac), ("win", win)):
        body.append(f'<div class="os" data-os="{profile}">')
        body.append('<ul class="notes">' + "".join(f"<li>{html.escape(note)}</li>" for note in NOTES[profile]) + "</ul>")
        body.append("<nav>" + "".join(
            f'<a href="#{profile}-{name}">{html.escape(spec["layerInfo"][name][0])}</a>'
            for name in LAYER_ORDER
        ) + "</nav>")
        body.extend(layer_html(spec, profile, name) for name in LAYER_ORDER)
        body.append("</div>")
    body.append('<p class="foot">淡い黄色＝短押しと長押しで操作が変わります。点線＝下のレイヤーを使うか割り当てなし。設定レイヤーは左 Shift ＋ \\ を押している間だけ有効です。</p></main><div id="tip"></div>')
    document = (
        '<!doctype html><html lang="ja"><head><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width,initial-scale=1">'
        '<title>Keyball44 キー操作早見表</title><link rel="icon" href="data:,"><style>' + CSS + "</style></head><body>"
        + "".join(body) + "<script>" + JS % json.dumps(combo_keys, ensure_ascii=False) + "</script></body></html>"
    )
    return document


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: gen_html.py MAC_SPEC.json WIN_SPEC.json OUT.html")
    mac = json.loads(Path(sys.argv[1]).read_text())
    win = json.loads(Path(sys.argv[2]).read_text())
    Path(sys.argv[3]).write_text(render(mac, win))


if __name__ == "__main__":
    main()
