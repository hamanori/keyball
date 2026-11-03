# Keyball44 `hamanori` ファームウェア

Keyball44用の個人ファームウェアと、現在のRemap設定を再現するためのマニュアルです。

## 1. 現在の運用

左右の半分には同じ `hamanori` ファームウェアを書き込みます。

OSを切り替えるときはRemapでキーを書き換えません。
USBケーブルを挿して親機にする側を切り替えます。

| USB接続して親機にする側 | 保存されている設定 | 接続先 |
| --- | --- | --- |
| 左半分 | Windows用、日本語配列 | Windows PC |
| 右半分 | macOS用、US英語配列 | Mac |

左右の半分を接続するケーブルは通常どおり接続したまま使います。
Remapの設定は左右それぞれの実機EEPROMに保存されています。

## 2. Remap設定の原本

2026-07-18時点で実機から出力したcheatsheetを、再現用の原本として保存しています。

- [Windows用、日本語配列](reference/remap-windows-japanese.pdf)
- [macOS用、US英語配列](reference/remap-mac-english-us.pdf)

再現用の機械可読データは、QMK標準の`keymap.json`形式でOSごとに分けています。

- [Windows用keymap JSON](reference/keymap-windows-japanese.json)
- [macOS用keymap JSON](reference/keymap-macos-english-us.json)

JSONの`layers`は、QMKの`LAYOUT_right_ball`へ渡せる順序で並んでいます。
各レイヤーは44要素で、物理位置の順序は`keyball44/info.json`のレイアウト定義に合わせています。

現時点では`Macro n`と`Any 0x...`だけ`KC_NO`にしています。
これらは後で置き換える対象としてJSONの`notes`にも記録しています。

PDFはキーの物理配置を確認するための正確な資料です。
本書では、PDFの読み方と各レイヤーの役割を整理します。

## 3. Remap cheatsheetの読み方

| 表記 | 意味 |
| --- | --- |
| `▽` | 透過キー。下位レイヤーのキーを使う |
| `Layer(n)` | そのキーを押している間、Layer `n` に入る |
| `*Ctrl`、`*Alt`、`*Win`、`*Shift` | 長押し時の修飾キー、または修飾キー付きの入力 |
| `(*S+7)` | Shift+7など、修飾キーを組み合わせた入力 |
| `Macro n` | Remap側に保存されたマクロ |
| `Any 0x...` | Remap側に保存された任意キーコード |
| `Kb n` | Keyball固有のキーコード |
| `User n` | このkeymapで定義したカスタムキーコード |

`Macro n` と `Any 0x...` は、RemapのEEPROMに保存された特殊入力です。
Remapを廃止してFWへ移植する場合は、対応するQMKキーコードまたはマクロに置き換えます。

## 4. レイヤー構成

| Layer | 役割 | 主な内容 | Windows/macOSの差分 |
| ---: | --- | --- | --- |
| 0 | 通常入力 | QWERTY、修飾キー、入力切替、Layer 1/2/3/4への入口 | 日本語配列とUS配列、修飾キー、記号、入力切替が異なる |
| 1 | 記号、編集、カーソル | 矢印、Home/End、括弧、コピー、ペースト、カーソル移動 | 記号位置とショートカットが大きく異なる |
| 2 | 数字、ファンクション | F1-F12、数字、記号、Win/Cmd付きショートカット | 数字列、記号列、修飾キーが異なる |
| 3 | アプリ操作、スクロール入口 | アプリ切替、タブ操作、ページ移動、Undo/Redo、Mute、マクロ | WindowsはCtrl/Alt中心、macOSはCtrl/Option/Command中心 |
| 4 | Shift入力 | Windows側では英字と記号のShift入力を配置 | Windows側に多数配置、macOS側はほぼ透過 |
| 5 | 未使用 | 全キー透過 | 差分なし |
| 6 | Keyball操作 | クリック、CPI、スクロール、その他のUserキー | 両方で共通 |

## 5. Windows用プロファイル

左半分をUSB接続して親機にしたときに使う設定です。

### Layer 0

- 日本語配列の記号を使います。
- `+`、`*S+7`、`¥` などを日本語配列の位置に配置しています。
- 親指付近に `英数`、`かな`、`Ctrl`、`Alt`、`Win` を配置しています。
- `Layer(1)`、`Layer(2)`、`Layer(3)`、`Layer(4)` の入口があります。

### Layer 1

- 矢印、Home、End、Print Screenを配置しています。
- `{`、`[`、`}`、`]` などの括弧を配置しています。
- Ctrl+C、Ctrl+Vなどの編集操作を配置しています。
- `Macro 11` と `Macro 12` があります。
- Ctrl、Alt、Shiftを組み合わせた入力があります。

### Layer 2

- F1-F12を配置しています。
- 数字列と日本語配列の記号を配置しています。
- `Win/Cmd 1`、`Win/Cmd 2`、`Win/Cmd 3` があります。
- `!`、`@`、`#`、`$`、`%`、`&`、`*`、`(`、`)`、`~` などをShift組み合わせで入力します。

### Layer 3

- Ctrl+1、Ctrl+2などのアプリ操作を配置しています。
- Alt+Tab、Alt+Page Up、Alt+Page Downを配置しています。
- Ctrl+Page Up、Ctrl+Page Down、Ctrl+W、Ctrl+Tを配置しています。
- Win/Cmd+矢印でウィンドウ操作を行います。
- Ctrl+Z、Ctrl+Y、Ctrl+J、Ctrl+0などを配置しています。
- Alt+左右、Alt+[、Alt+]、Muteがあります。
- `Macro 0`、`Macro 1`、`Macro 2`、`Macro 3` と `Any 0x...` の入力があります。

### Layer 4

- 英字をShift付きで入力します。
- 日本語配列の記号をShift付きで入力します。
- Shift、Ctrl、Altを組み合わせた入力があります。
- macOSプロファイルとは異なり、独立したShift入力レイヤーとして使います。

### Layer 5

すべて透過です。

### Layer 6

macOSプロファイルと同じKeyball操作用の配置です。

## 6. macOS用プロファイル

右半分をUSB接続して親機にしたときに使う設定です。

### Layer 0

- US英語配列の記号を使います。
- `-`、`\`、`Lang 1`、`Lang 2` を配置しています。
- Command、Option、Control、ShiftをmacOS向けの位置に配置しています。
- `Layer(1)`、`Layer(2)`、`Layer(3)` の入口があります。

### Layer 1

- 数字とUS英語配列の記号を配置しています。
- 矢印、Home、Endを配置しています。
- Shift+Wなど、編集用のショートカットがあります。
- 括弧、`+`、`=`、`_` などをUS配列に合わせて配置しています。

### Layer 2

- F1-F12を配置しています。
- US英語配列の数字と記号を配置しています。
- `!`、`@`、`#`、`$`、`%`、`^`、`&`、`*`、`(`、`)`、`~` などをShift組み合わせで入力します。
- Windowsプロファイルとは数字、記号、修飾キーの位置が異なります。

### Layer 3

- Command、Control、Optionを使ったアプリ操作を配置しています。
- Command+Q、Command+W、Command+Zなどを配置しています。
- Control+矢印、Option+矢印で移動操作を行います。
- Control+Page Up、Control+Page Down、Control+W、Control+Fなどがあります。
- Undo/Redo、タブ操作、Muteがあります。
- `Any 0x...` の入力があります。

### Layer 4

ほぼすべて透過です。
WindowsプロファイルのようなShift専用配列は持ちません。

### Layer 5

すべて透過です。

### Layer 6

Windowsプロファイルと同じKeyball操作用の配置です。

## 7. Keyball固有の操作

Remap設定とは別に、FW側にはトラックボール操作を制御する処理があります。

### クリックレイヤー

1. 通常状態でトラックボールを動かすと、Mouse Session ModuleがLayer 6を有効にします。
2. Layer 6では、右手ホームポジション付近でマウスボタンを押せます。
3. 最後の有意なボール移動から10秒間は、クリック操作を待てます。
4. Ctrl、Shift、Alt、CommandまたはWinを押してもLayer 6を維持します。
5. その他のキーを押すとLayer 6を解除し、そのキーを通常入力として送ります。
6. マウスボタンまたはスクロールキーを押している間は、時間経過では解除しません。
7. 通常クリック後は500msだけ連続クリックを待ちます。

クリックは、既存のRemap設定との互換性を保つために`User 0-4`を入口として残しています。
FW内部ではQMK標準の`KC_MS_BTN1-5`へ変換し、標準のPointing Device処理から送信します。
ドラッグまたはスクロールの終了後は、ボール停止を確認するまでLayer 6を再起動しません。

詳細な状態遷移と受け入れ条件は、[マウスセッション仕様](mouse_session_spec.md)に記載しています。

クリックレイヤーの主なキーは次のとおりです。

| キー | 役割 |
| --- | --- |
| `User 0` | 左クリック相当 |
| `User 1` | 右クリック相当 |
| `User 3` | 戻る相当 |
| `User 4` | 進む相当 |
| `User 5` | クリックレイヤー中のスクロール |

### スクロール

スクロールには2つの入口があります。

- Layer 0の `Layer(3)` をholdしてLayer 3に入る。
- クリックレイヤー中の `User 5` を押しながらボールを動かす。

Y方向の動きがX方向の半分以上ある場合は縦スクロールとして扱います。
横方向のしきい値は縦方向の2倍です。

現在のFWでは、Windows/Linuxはスクロール方向を反転し、macOS/iOSは反転しません。

### OLED

OLEDには次の状態を表示します。

| 表示 | 意味 |
| --- | --- |
| `NONE` | 通常状態 |
| `CLKB` | クリックレイヤーが有効 |
| `CLCK` | マウスボタンを押している |
| `SCRL` | miniZoneスクロール中 |

Layer、CPI、スクロールしきい値、OS判定も表示します。

## 8. 実行時に調整できる値

| 項目 | 初期値 | 範囲または刻み | 保存先 | 現在の操作方法 |
| --- | ---: | --- | --- | --- |
| `scroll_threshold` | 30 | 1-200、5刻み | User EEPROM | Layer 5の調整キー。ただしLayer 5への入口がない |
| Keyball CPI | 700 | 100-12000、100または1000刻み | Keyball EEPROM | `Kb 2`、`Kb 3` など。cheatsheetでは配置あり |
| Keyball scroll divider | 4 | 1-7 | Keyball EEPROM | 標準キーコード。独自miniZoneスクロールとは別 |
| マウス操作待機時間 | 10000ms | FW起動時に固定 | `config.h` | `MOUSE_SESSION_READY_TIMEOUT`を変更して再ビルド |
| 連続クリック猶予 | 500ms | FW起動時に固定 | `config.h` | `MOUSE_SESSION_MULTI_CLICK_TERM`を変更して再ビルド |

Remapを削除する場合、これらの調整を残すか、FWの定数として管理するかを決めます。

## 9. コンパイル時に調整する値

| 設定 | 現在値 | ファイル | 意味 |
| --- | --- | --- | --- |
| `TAPPING_TERM` | 200 | `config.h` | tap/hold判定時間 |
| `QUICK_TAP_TERM` | 0 | `config.h` | quick tapのhold化待ち時間 |
| `TAP_CODE_DELAY` | 5 | `config.h` | tapコード送信の遅延 |
| `MOUSE_SESSION_READY_TIMEOUT` | 10000 | `config.h` | 最後の有意な移動または修飾キー押下からLayer 6を解除するまでの時間 |
| `MOUSE_SESSION_MULTI_CLICK_TERM` | 500 | `config.h` | 通常クリック後に連続クリックを待つ時間 |
| `MOUSE_SESSION_ACTIVATION_DISTANCE` | 6 | `config.h` | マウスセッションを開始する移動量 |
| `MOUSE_SESSION_ACTIVATION_WINDOW` | 100 | `config.h` | 起動用の移動量を累積する時間窓 |
| `MOUSE_SESSION_DRAG_THRESHOLD` | 6 | `config.h` | 通常クリックとドラッグを分ける移動量 |
| `MOUSE_SESSION_BALL_STOP_REPORTS` | 2 | `config.h` | ボール停止と判定するゼロ入力の連続回数 |
| `DYNAMIC_KEYMAP_LAYER_COUNT` | 7 | `config.h` | VIA/Remapで扱うレイヤー数 |
| `RGBLIGHT_ENABLE` | yes | `rules.mk` | RGB制御 |
| `OLED_ENABLE` | yes | `rules.mk` | OLED表示 |
| `VIA_ENABLE` | yes | `rules.mk` | VIA/Remap対応 |
| `EXTRAKEY_ENABLE` | yes | `rules.mk` | メディアキーなどの追加キー |
| `OS_DETECTION_ENABLE` | yes | `rules.mk` | USB初期化挙動からOSを推測 |

## 10. PRD

### 目的

Keyball44を、トラックボールによるポインタ移動、クリック、スクロールまでキーボード上で完結する常用入力デバイスにする。

### 対象

- Keyball44をメインキーボードとして使う。
- Windowsでは日本語配列を使う。
- macOSではUS英語配列を使う。
- OS切り替え時にRemapで設定を書き換えない。

### 要件

| ID | 要件 | 現状 |
| --- | --- | --- |
| REQ-1 | 通常文字入力ができる | 実装済み |
| REQ-2 | トラックボール移動後にクリック可能状態へ入る | 実装済み |
| REQ-3 | 左右クリック、戻る、進むをキーボード上で操作できる | 実装済み |
| REQ-4 | ボール移動を縦横スクロールに変換できる | 実装済み |
| REQ-5 | WindowsとmacOSで別のキーマップを選べる | 現在は左右のEEPROM設定で実現 |
| REQ-6 | Windowsは日本語配列、macOSはUS英語配列になる | Remap設定で実現 |
| REQ-7 | ファームウェアをローカルでビルドできる | 実装済み |
| REQ-8 | RemapなしでOS判定とキーマップ切替を行う | 次の実装候補 |

### Remap廃止後の実装案

Windows用とmacOS用でLayer 0-4の差分が大きいため、修飾キーだけを変換する方式では現在の設定を完全には再現できません。

Remapを廃止した後は、1つのFWにWindows用ベースレイヤーとmacOS用ベースレイヤーを持たせます。
USB親機側でOSを判定し、判定後に標準レイヤーを切り替えます。

分割キーボードでは、親機だけがOSを判定します。
左右の入力で同じ判定結果を使うため、`SPLIT_DETECTED_OS_ENABLE` を有効にしてOS判定結果を分割先へ同期します。

QMKのOS判定はUSB初期化時の挙動から推測するため、ハブ、ドック、KVM経由では誤判定する可能性があります。
`OS_UNSURE` 時の既定プロファイルか、手動切替キーを残します。

## 11. 削除候補

現在のビルドサイズは`28506 / 28672 bytes`で、残りは166 bytesです。
新機能を追加する前に、不要な機能を削除します。

QMK標準Auto Mouseは無効にし、Layer 6の有効化と解除をMouse Session Moduleへ集約しています。

### 優先度が高い候補

| 候補 | 理由 | リスク |
| --- | --- | --- |
| `VIA_ENABLE` | Remapを廃止するなら不要 | 実機上での配置変更ができなくなる |
| `DYNAMIC_KEYMAP_LAYER_COUNT` | VIA/Remap用の動的レイヤー管理に必要 | 静的keymapへ移行する必要がある |
| `KC_TO_CLICKABLE_INC/DEC` | 処理は削除済み。RemapのUser番号を維持するため値だけ予約 | Remap廃止前に削除するとUser 8-10がずれる |
| Layer 4/5 | 現在のFWでは未使用または入口がない | Remapの自由度が下がる |

### 維持判断が必要な候補

- `OLED_ENABLE`：状態確認に使うため、開発中は維持する。
- `RGBLIGHT_ENABLE`：LEDを使わないなら削除候補。
- `EXTRAKEY_ENABLE`：メディアキーなどを使わないなら削除候補。
- `OS_DETECTION_ENABLE`：OS別キーマップ切替に使うため、移行後も維持する。

## 12. 再現手順

### 現在のRemap運用を再現する

1. 左右の半分に同じ `hamanori` FWを書き込む。
2. Windows PCに左半分をUSB接続して親機にする。
3. Remapで [Windows用PDF](reference/remap-windows-japanese.pdf) を見ながらLayer 0-6を設定する。
4. 設定を保存する。
5. Macに右半分をUSB接続して親機にする。
6. Remapで [macOS用PDF](reference/remap-mac-english-us.pdf) を見ながらLayer 0-6を設定する。
7. 設定を保存する。
8. 左右を分割ケーブルで接続する。
9. Windowsでは左半分を、macOSでは右半分をUSB接続して親機にする。

### FWへ移植した後の確認

1. `qmk compile` が成功する。
2. WindowsでOS判定がWindowsになることをOLEDで確認する。
3. Windowsで日本語配列、入力切替、Layer 1-4の主要ショートカットを確認する。
4. MacでOS判定がmacOSになることをOLEDで確認する。
5. MacでUS英語配列、入力切替、Layer 1-4の主要ショートカットを確認する。
6. 両OSでクリック、スクロール、CPI操作を確認する。
7. `OS_UNSURE` 時の既定プロファイルを確認する。

## 13. ローカルビルド

```sh
cd /Users/hamanori/Development/individual/keyball
qmk compile
```

明示的に指定する場合は次のコマンドを使います。

```sh
qmk compile -kb keyball/keyball44 -km hamanori
```

生成されるファイルは次の場所です。

```text
/Users/hamanori/qmk_firmware/keyball_keyball44_hamanori.hex
```

書き込みは次のコマンドです。

```sh
qmk flash
```

Keyball44はsplit keyboardなので、左右をそれぞれ接続してリセットし、同じFWを片側ずつ書き込みます。
ブートローダーはCaterinaです。

## 14. 参照元

- `qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori/keymap.c`
- `qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori/config.h`
- `qmk_firmware/keyboards/keyball/keyball44/keymaps/hamanori/rules.mk`
- `qmk_firmware/keyboards/keyball/lib/keyball/keyball.c`
- `qmk_firmware/keyboards/keyball/lib/keyball/keyball.h`
- [Windows用Remap cheatsheet](reference/remap-windows-japanese.pdf)
- [macOS用Remap cheatsheet](reference/remap-mac-english-us.pdf)
- [Windows用QMK keymap JSON](reference/keymap-windows-japanese.json)
- [macOS用QMK keymap JSON](reference/keymap-macos-english-us.json)
- [JSON生成スクリプト](tools/extract_remap_cheatsheet.py)
