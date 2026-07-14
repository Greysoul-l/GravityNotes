# GravityNotes 入力仕様および実装ドキュメント

本ドキュメントでは、GravityNotesにおける入力抽象化の仕様、各シーンへの実装詳細、および使用する `Input_Action` 定数について解説します。

---

## 1. 入力抽象化の概要

接続されたコントローラーや入力タイプに関わらず、すべての入力は `InputManager`（`input_manager.h` / `input_manager.cpp`）を介して抽象化されています。

*   **任天堂配列 (ABXY) 強制**: コントローラーの種類に関わらず、任天堂の標準的な配置（右ボタンがA、下ボタンがB）が適用されます。
*   **直接インクルードの排除**: 各シーンのソースファイルからは `keyboard.h` や `gamepad.h` の直接的なインクルードを排除し、すべて `input_manager.h` のみを使用しています。

---

## 2. アクション定数（Input_Action）とマッピング仕様

ゲームで使用される `Input_Action` 定数と、キーボードおよびゲームパッドの具体的な割り当ては以下の通りです。

| 定数名 | 主な用途 | キーボード割り当て | ゲームパッド / アナログ割り当て |
| :--- | :--- | :--- | :--- |
| `INPUT_ACTION_DECIDE` | 決定 / 進行 | `Space`, `Enter` | `Aボタン` |
| `INPUT_ACTION_CANCEL` | キャンセル / 戻る | `BackSpace` | `Bボタン` |
| `INPUT_ACTION_MENU_UP` | メニュー上の選択 | `W`, `↑ 矢印キー` | `DPad-UP` / `LStick-UP` (※1) |
| `INPUT_ACTION_MENU_DOWN` | メニュー下の選択 | `S`, `↓ 矢印キー` | `DPad-DOWN` / `LStick-DOWN` (※1) |
| `INPUT_ACTION_MENU_LEFT` | メニュー左の選択 | `A`, `← 矢印キー` | `DPad-LEFT` / `LStick-LEFT` (※1) |
| `INPUT_ACTION_MENU_RIGHT` | メニュー右の選択 | `D`, `→ 矢印キー` | `DPad-RIGHT` / `LStick-RIGHT` (※1) |
| `INPUT_ACTION_PAUSE` | ポーズメニュー表示 | `Escape` | `STARTボタン` |
| `INPUT_ACTION_ATTACK` | 攻撃 / ノーツタップ | `Space` | `LT` (>0.5f), `RT` (>0.5f), `LB`, `RB` |
| `INPUT_ACTION_MOVE_UP` | プレイヤー移動・上 | `W` | `DPad-UP` / `LStick-UP` (※1) |
| `INPUT_ACTION_MOVE_DOWN` | プレイヤー移動・下 | `S` | `DPad-DOWN` / `LStick-DOWN` (※1) |
| `INPUT_ACTION_MOVE_LEFT` | プレイヤー移動・左 | `A` | `DPad-LEFT` / `LStick-LEFT` (※1) |
| `INPUT_ACTION_MOVE_RIGHT` | プレイヤー移動・右 | `D` | `DPad-RIGHT` / `LStick-RIGHT` (※1) |
| `INPUT_ACTION_GRAVITY_UP` | 重力移動（天井） | `↑ 矢印キー` | `RStick-UP` (※1) |
| `INPUT_ACTION_GRAVITY_DOWN` | 重力移動（床） | `↓ 矢印キー` | `RStick-DOWN` (※1) |
| `INPUT_ACTION_GRAVITY_LEFT` | 重力移動（左壁） | `← 矢印キー` | `RStick-LEFT` (※1) |
| `INPUT_ACTION_GRAVITY_RIGHT`| 重力移動（右壁） | `→ 矢印キー` | `RStick-RIGHT` (※1) |
| `INPUT_ACTION_DEBUG_F1` | 強制リスタート | `F1` | （なし） |

> **(※1) スティックトリガー検出について**: 
> メニュー移動・レーン移動・重力変更など「1回倒した瞬間」のみ入力を受け取る処理を適切に行うため、`InputManager` 内部でスティックの現在値と前フレームの値を比較し、閾値（`0.5f`）を超えた最初の1フレームだけを検知するトリガー機構を実装しています。これにより長押しによる誤作動を防いでいます。また、アナログトリガーである `LT` / `RT` による攻撃判定（`INPUT_ACTION_ATTACK`）についても同様にトリガー化されています。

---

## 3. 各シーンにおける入力処理の実装詳細

### SCENE_TITLE (タイトル)
*   **入力の絡む処理**: 決定操作があった場合に `SetSceneFade(SCENE_STAGESELECT)` でステージセレクト画面に遷移します。
*   **使用する抽象入力**: `INPUT_ACTION_DECIDE`

### SCENE_STAGESELECT (ステージセレクト)
*   **入力の絡む処理**:
    *   曲（ディスク）の選択: プレイヤーが上下に入力した際に、ターンテーブル上のアルバムを切り替えます。
    *   曲の決定: 選択中の曲が再生可能な状態で決定操作があった場合に、譜面情報を設定して `SCENE_GAME` へ遷移します。
*   **使用する抽象入力**:
    *   曲選択: `INPUT_ACTION_MENU_UP` / `INPUT_ACTION_MENU_DOWN`
    *   曲決定: `INPUT_ACTION_DECIDE`

### SCENE_GAME (ゲーム本編)
*   **入力の絡む処理**:
    *   攻撃（ノーツタップ）: 押した瞬間にノーツのヒット判定を行い、ホールドノーツの場合は押し続け、ロープノーツで手を離した瞬間にリリース判定を行います。
    *   上下左右移動: 現在プレイヤーが立っている面（床・天井・左右壁）を基準にした、平面上の横跳び移動を行います。
    *   重力移動: 現在の重力方向から、指示された新しい面（天井・床・左壁・右壁）に向かって重力を変更します。
    *   ポーズ（未実装）: `INPUT_ACTION_PAUSE` に割り当てられる予定です。
    *   デバッグリスタート: `F1`キーを押した瞬間に、`options.yml` の `startmeasure` からゲームをリスタートします。
*   **使用する抽象入力**:
    *   攻撃: `INPUT_ACTION_ATTACK`（`Input_IsActionTrigger` でのヒット判定、`Input_IsActionDown` でのホールド/リリース判定）
    *   プレイヤー移動: `INPUT_ACTION_MOVE_LEFT` / `RIGHT` / `UP` / `DOWN` (プレイヤーの重力面に応じて自動割り当て)
    *   重力変更: `INPUT_ACTION_GRAVITY_UP` / `DOWN` / `LEFT` / `RIGHT`
    *   デバッグ強制リスタート: `INPUT_ACTION_DEBUG_F1`

### SCENE_RESULT (リザルト)
*   **入力の絡む処理**: 決定操作があった場合に `SetPlayJson("")` でリザルトデータを初期化し、`SetSceneFade(SCENE_STAGESELECT)` でステージセレクトに戻ります。
*   **デバッグ Rキーによるリスタート**: `result.cpp` にあったデバッグ用のRキーによるカウントリスタート処理は、直接インクルード削除に伴い**コメントアウト**されています。
*   **使用する抽象入力**: `INPUT_ACTION_DECIDE`
