# Keyball Firmware Context

## Glossary

### Auto Mouse Layer

ボール操作に応じて一時的に有効になり、クリックやスクロール操作を提供するレイヤー。

### Intentional Pointer Motion

利用者がポインターを移動する目的で行ったボール操作。

### Motion Noise

打鍵や筐体の振動によってセンサーが検出した、利用者が意図していない微小なボール移動。

### Activation Policy

検出したボール移動をIntentional Pointer Motionと判断し、Auto Mouse Layerを有効にしてよいか決める規則。

### Layer Ownership

Auto Mouse Layerの有効化、維持、解除を一つの仕組みだけが管理するという責務。QMK Auto Mouseを唯一の所有者とし、独自処理はActivation Policyを提供する。

### Keyboard Suppression Window

キーボード操作を優先するため、Auto Mouse Layerの新規起動を抑止する期間。

### Mouse Operation Key

クリック、ドラッグ、スクロールなど、Auto Mouse Layer上でポインティング操作を継続するキー。

### Keyboard Intent

文字入力、ショートカット、または手動レイヤー操作を行う意図。Auto Mouse Layerが有効な場合でも、対応する入力は通常キーマップとして解釈される必要がある。

### Modified Mouse Operation

Control、Shift、Option、Commandのいずれかを伴うクリック、ドラッグ、またはスクロール操作。

### Click Stabilization

クリック操作による筐体やボールの微動でポインター位置がずれることを抑える振る舞い。ドラッグ開始を妨げないことと両立する必要がある。
