"""Render the Mac spec as a self-contained ortholinear cheat sheet.

usage: gen_html.py SPEC.json OUT.html [proposal]
"""
import html, json, sys

spec = json.load(open(sys.argv[1]))
OUT = sys.argv[2]
PROPOSAL = len(sys.argv) > 3 and sys.argv[3] == 'proposal'


def grid_pos(p):
    r, c = map(int, p.split(','))
    if r < 3:
        return r, c
    if 3 < r < 7:
        return r - 4, 13 - c
    return 3, {(3, 1): 2, (3, 2): 3, (3, 3): 4, (3, 4): 5, (3, 5): 6, (7, 5): 7, (7, 4): 8, (7, 1): 12}[(r, c)]


COLORS = ['#c2410c', '#0f766e', '#7c3aed', '#be185d', '#1d4ed8', '#4d7c0f', '#b45309', '#0e7490', '#9333ea', '#dc2626', '#047857']


def key_html(layer, p, k, badges=()):
    row, col = grid_pos(p)
    col += col >= 7  # skip the split gap column
    cls = ['key', k['kind']]
    if row == 3:
        cls.append('thumb')
    label = html.escape(k['label'])
    detail = html.escape(k['detail'])
    small = ' long' if len(k['label']) > 5 else ''
    b = ''.join('<span class="badge" style="--c:%s" data-combo="%d">%s</span>' % (c, i, html.escape(s)) for i, s, c in badges)
    return ('<div class="%s" style="grid-row:%d;grid-column:%d" data-pos="%s" data-detail="%s" tabindex="0">'
            '<span class="lbl%s">%s</span>%s</div>') % (' '.join(cls), row + 1, col + 1, p, detail, small, label, b)


def layer_html(name):
    title, how = spec['layerInfo'][name]
    keys = spec['layers'][name]
    items, legend = [], []
    badges = {}
    if name == 'base':
        cross = 0
        ported = [c for c in spec['combos'] if c['ported']] + [dict(spec['toggle'], ported=True)]
        adjacent = {}
        for i, cb in enumerate(ported):
            (ra, ca), (rb, cb_) = sorted(map(grid_pos, cb['pos']), key=lambda g: g[1])
            if ra == rb and cb_ - ca == 1:
                adjacent[i] = (ra, ca)
        # Pairs sharing a key alternate their tab above/below so both stay readable.
        side = {}
        for i, (r, c) in sorted(adjacent.items(), key=lambda kv: kv[1]):
            left = [j for j, (r2, c2) in adjacent.items() if r2 == r and c2 == c - 1]
            side[i] = 'down' if left and side.get(left[0]) == 'up' else 'up'
        for i, cb in enumerate(ported):
            a, b = sorted(cb['pos'], key=lambda p: grid_pos(p)[1])
            names = [keys[a]['label'], keys[b]['label']]
            if i in adjacent:
                r, c = adjacent[i]
                c += c >= 7
                items.append('<div class="combo %s" style="grid-row:%d;grid-column:%d / span 2" data-combo="%d">'
                             '<span class="ctab">%s</span></div>' % (side[i], r + 1, c + 1, i, html.escape(cb['symbol'])))
                color = None
            else:
                color = COLORS[cross % len(COLORS)]
                cross += 1
                for p in (a, b):
                    badges.setdefault(p, []).append((i, cb['symbol'], color))
            what = '：' + cb['label'] if 'label' in cb else ''
            legend.append('<li data-combo="%d" tabindex="0"><b%s>%s</b> %s ＋ %s%s</li>' % (
                i, ' style="color:%s"' % color if color else '', html.escape(cb['symbol']), html.escape(names[0]), html.escape(names[1]), html.escape(what)))
    for p, k in keys.items():
        items.append(key_html(name, p, k, badges.get(p, ())))
    out = ['<section class="layer" id="layer-%s"><h2>%s<small>%s</small></h2>' % (name, title, html.escape(how)),
           '<div class="board-wrap"><div class="board">%s</div></div>' % ''.join(items)]
    if legend:
        out.append('<div class="legend"><h3>コンボ（2つのキーを同時に押す・%dms 以内）</h3><p>同じ手の組は枠、左右をまたぐ組は同じ色の記号で示しています。</p><ul>%s</ul></div>' % (spec['comboTermMs'], ''.join(legend)))
    out.append('</section>')
    return ''.join(out)


PROPOSAL_HTML = '''
<section class="note">
<h2>配置案のポイント</h2>
<ul>
<li>指の位置は CoreX と同じです。3段×左右6列は全レイヤーで同じキーを同じ場所に置きます。</li>
<li>コンボ 23 個のうち 23 個を移します（受付 30ms）。CoreX の「左Shift＋\\ で接続設定」は Keyball に接続設定がないので移しません。</li>
<li>英数の長押しは CoreX と同じ 250ms 判定、ほかの長押しは 200ms です。</li>
<li>マウスレイヤーは今の Keyball の仕組み（ボールを動かすと入り、文字キーで抜ける）を使います。CoreX のジェスチャーキーの位置は「押している間スクロール」にします。</li>
<li>Windows 用は今の設定をそのままファームに固定し、接続先の OS で自動的に切り替えます。Windows への CoreX 移植は後日です。</li>
</ul>
<h2>CoreX と違うところ</h2>
<table>
<tr><th>CoreX</th><th>Keyball</th></tr>
<tr><td>左下 Ctrl・Option・⌘（3キー）</td><td>親指上の2キーに ⌥ Option・⌘ Command。Ctrl は右下へ</td></tr>
<tr><td>右下 Spotlight（/ の下）</td><td>⌃ Control。Spotlight は ⌘＋Space で開く <em>要判断</em></td></tr>
<tr><td>右下 右Option、右Shift（長押しウィンドウ）</td><td>キーが足りないので置かない。Shift は Space 長押し、ウィンドウは \\ か英数の長押し</td></tr>
<tr><td>左ダイヤル（音量・押して消音）</td><td>数字・記号レイヤーの左端列に 音量＋・音量−・消音</td></tr>
<tr><td>右ダイヤル（スクロール）</td><td>ウィンドウレイヤーを押している間、またはマウス中に「スクロール」キーを押している間、ボールでスクロール</td></tr>
<tr><td>ダイヤル押し（中クリック）</td><td>マウスレイヤーの K 短押し</td></tr>
<tr><td>ジェスチャーキー</td><td>同じ位置を「押している間スクロール」に</td></tr>
<tr><td>BLE 接続（L5）、接続設定（L6）</td><td>有線なので移さない</td></tr>
<tr><td>長押し判定「balanced」</td><td>QMK の Permissive Hold（同じ考え方）を Mac 用にだけ使う <em>要判断</em></td></tr>
</table>
<h2>OS 側の設定</h2>
<ul>
<li>F13・F18・F19（ウィンドウ配置）と ⌃⌥⌘＋7/8/9（Dia・Codex・Claude を開く）は、Mac 側のショートカット設定がそのまま効きます。追加の設定はいりません。</li>
<li>macOS で Keyball の配列が US として認識されていることを、システム設定 → キーボード で確認してください。</li>
<li>Karabiner-Elements は今のまま Keyball を無視する設定にしておきます。</li>
</ul>
<h2>実機で確かめること</h2>
<ul>
<li>OLED の OS 表示が Mac につないだとき MAC、Windows につないだとき WIN になること</li>
<li>コンボで記号が出ること、通常の速い打鍵でコンボが誤って出ないこと</li>
<li>マウス中の K：短押しで中クリック、長押しでウィンドウレイヤー</li>
</ul>
</section>'''

CSS = '''
:root{--bg:#f7f7f5;--fg:#1a1a1a;--muted:#666;--key:#fff;--keyb:#d4d4d0;--thumb:#eef6f5;--dual:#fff8eb;--trns:#f1f1ef;--accent:#0f766e;--combo:#c2410c;--card:#fff}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--bg:#161616;--fg:#eee;--muted:#aaa;--key:#262626;--keyb:#3a3a3a;--thumb:#1f2e2c;--dual:#2e2717;--trns:#1d1d1d;--accent:#5eead4;--combo:#fb923c;--card:#1e1e1e}}
:root[data-theme="dark"]{--bg:#161616;--fg:#eee;--muted:#aaa;--key:#262626;--keyb:#3a3a3a;--thumb:#1f2e2c;--dual:#2e2717;--trns:#1d1d1d;--accent:#5eead4;--combo:#fb923c;--card:#1e1e1e}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font-family:"Noto Sans JP","Hiragino Sans",system-ui,sans-serif;line-height:1.5}
main{max-width:1180px;margin:0 auto;padding:16px}
h1{font-size:1.5rem;margin:.5rem 0}h2{font-size:1.15rem;margin:1.6rem 0 .5rem}h2 small{font-weight:400;color:var(--muted);font-size:.85rem;margin-left:.75rem}
.lead{color:var(--muted);margin:0 0 1rem}
nav{display:flex;gap:.5rem;flex-wrap:wrap;margin:1rem 0}nav a{color:var(--accent);text-decoration:none;border:1px solid var(--keyb);border-radius:999px;padding:.2rem .8rem;font-size:.9rem}
.board-wrap{overflow-x:auto;padding:14px 0 18px}
.board{--u:clamp(40px,5.2vw,66px);display:grid;grid-template-columns:repeat(7,var(--u)) 18px repeat(7,var(--u));grid-template-rows:repeat(4,var(--u));gap:6px;width:max-content;position:relative}
.key{position:relative;background:var(--key);border:1px solid var(--keyb);border-radius:10px;display:flex;align-items:center;justify-content:center;text-align:center;padding:3px;cursor:default;outline:none}
.key.thumb{background:var(--thumb)}.key.dual{background:var(--dual)}.key.trns,.key.none{background:var(--trns);border-style:dashed}
.key:hover,.key:focus,.key.hl{border-color:var(--accent);box-shadow:0 0 0 2px var(--accent)}
.lbl{font-weight:700;font-size:clamp(.72rem,1.25vw,.98rem);line-height:1.2;word-break:keep-all;overflow-wrap:anywhere}.lbl.long{font-size:clamp(.62rem,1.05vw,.82rem)}
.badge{position:absolute;bottom:3px;right:3px;background:var(--c);color:#fff;font-size:.72rem;font-weight:700;border-radius:999px;min-width:18px;height:18px;line-height:18px;padding:0 4px;text-align:center}
.badge+.badge{right:24px}
.combo{position:relative;border:2px solid var(--combo);border-radius:12px;pointer-events:none;z-index:2;margin:-4px}
.combo.down{margin:4px -2px -6px;border-style:dashed}
.ctab{position:absolute;left:50%;transform:translateX(-50%);background:var(--combo);color:#fff;font-weight:700;font-size:.8rem;border-radius:6px;padding:0 6px;line-height:1.35}
.combo.up .ctab{top:-12px}.combo.down .ctab{bottom:-12px}
.combo.hl{border-width:3px;box-shadow:0 0 0 3px color-mix(in srgb,var(--combo) 30%,transparent)}
.board .key:nth-child(n){grid-column-end:auto}
.legend ul{list-style:none;padding:0;display:flex;flex-wrap:wrap;gap:.4rem .9rem;margin:.3rem 0}.legend li{font-size:.9rem;cursor:default}.legend li b{display:inline-block;min-width:1.2em;font-size:1.05rem}
.legend p{color:var(--muted);font-size:.85rem;margin:.2rem 0}
.legend h3{font-size:1rem;margin:.5rem 0 .2rem}
#tip{position:fixed;z-index:10;max-width:280px;background:var(--fg);color:var(--bg);font-size:.85rem;padding:.45rem .65rem;border-radius:8px;pointer-events:none;display:none}
.note{background:var(--card);border:1px solid var(--keyb);border-radius:12px;padding:4px 18px 14px;margin:1rem 0}
.note table{border-collapse:collapse;width:100%;font-size:.9rem}.note td,.note th{border-top:1px solid var(--keyb);padding:.35rem .5rem;text-align:left;vertical-align:top}
.note em{font-style:normal;color:var(--combo);font-weight:700;white-space:nowrap}
.foot{color:var(--muted);font-size:.8rem;margin-top:2rem}
@media print{nav,.note{display:none}.layer{break-inside:avoid}}
'''

JS = '''
const tip=document.getElementById('tip');
function show(el,x,y){tip.textContent=el.dataset.detail;tip.style.display='block';const w=tip.offsetWidth,h=tip.offsetHeight;tip.style.left=Math.min(innerWidth-w-8,Math.max(8,x-w/2))+'px';tip.style.top=(y-h-12<8?y+16:y-h-12)+'px'}
function hide(){tip.style.display='none'}
document.querySelectorAll('.key').forEach(k=>{
 k.addEventListener('mouseenter',e=>{const r=k.getBoundingClientRect();show(k,r.left+r.width/2,r.top)});
 k.addEventListener('mouseleave',hide);
 k.addEventListener('focus',()=>{const r=k.getBoundingClientRect();show(k,r.left+r.width/2,r.top)});
 k.addEventListener('blur',hide);
});
function hl(id,on){document.querySelectorAll('[data-combo="'+id+'"]').forEach(e=>{e.classList.toggle('hl',on);const k=e.closest('.key');if(k)k.classList.toggle('hl',on)})}
const combos=%s;
document.querySelectorAll('.legend li,.badge').forEach(li=>{
 const id=li.dataset.combo;
 const keys=()=>combos[id].map(p=>document.querySelector('#layer-base .key[data-pos="'+p+'"]'));
 li.addEventListener('mouseenter',()=>{hl(id,true);keys().forEach(k=>k&&k.classList.add('hl'))});
 li.addEventListener('mouseleave',()=>{hl(id,false);keys().forEach(k=>k&&k.classList.remove('hl'))});
 li.addEventListener('focus',()=>li.dispatchEvent(new Event('mouseenter')));li.addEventListener('blur',()=>li.dispatchEvent(new Event('mouseleave')));
});
'''

ported = [c['pos'] for c in spec['combos'] if c['ported']] + [spec['toggle']['pos']]
order = ['base', 'edit', 'mouse', 'window', 'num']
title = 'Keyball 配置案（Mac）' if PROPOSAL else 'Keyball キー操作早見表'
body = ['<main><h1>%s</h1>' % title,
        '<p class="lead">%s</p>' % ('CoreX の現行設定を Keyball44（Mac・US 配列）へ移す配置案です。まだ本体には書き込んでいません。キーにカーソルを合わせると、送るショートカットや短押し／長押しの説明が出ます。' if PROPOSAL else
                                     'Mac・US 配列で使う Keyball44 の現行設定です。Windows につないでも同じコンボで同じ記号が出ます（左 Shift＋\\ で Mac／Windows を手動で切り替え）。キーにカーソルを合わせると、送るショートカットや短押し／長押しの説明が出ます。'),
        '<nav>%s</nav>' % ''.join('<a href="#layer-%s">%s</a>' % (n, spec['layerInfo'][n][0]) for n in order)]
if PROPOSAL:
    body.append(PROPOSAL_HTML)
body += [layer_html(n) for n in order]
body.append('<p class="foot">薄い青緑＝親指キー、淡い黄色＝短押しと長押しで別の操作、点線＝下のレイヤーと同じか割り当てなし。</p></main><div id="tip"></div>')
doc = '<!doctype html><html lang="ja"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>%s</title><style>%s</style></head><body>%s<script>%s</script></body></html>' % (
    'Keyball 配置案' if PROPOSAL else 'Keyball 早見表', CSS, ''.join(body), JS % json.dumps(ported))
open(OUT, 'w').write(doc)
print('wrote', OUT, len(doc))
