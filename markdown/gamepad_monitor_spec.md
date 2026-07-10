# GravityNotes GamePad & Input Monitor 仕様解説書

本ドキュメントでは、GravityNotesフレームワークにおけるゲームパッド入力システム（`gamepad.h` / `gamepad.cpp`）、それをゲームアクションに仲介する `InputManager`（`input_manager.h` / `input_manager.cpp`）、およびデバッグ用の入力監視コンソール `InputMonitorConsole`（`input_monitor_console.h` / `input_monitor_console.cpp`）の設計と使用方法について解説します。

---

## 1. 概要

GravityNotesは、Windowsプラットフォームにおける多様なコントローラー入力を処理するために、以下の3つのレイヤーで入力処理を行っています。

1. **GamePadモジュール**: XInput, RawInput, DirectInput の3つのAPIをサポートし、接続中のデバイスや直近の入力アクティビティを監視して最適なバックエンドを自動決定します。
2. **InputManager**: GamePadとキーボードからの入力を統合し、ゲーム内の抽象アクション（決定、キャンセル、移動ベクトル等）へマッピングします。
3. **InputMonitorConsole**: デバッグビルド（`_DEBUG`）時において、接続中のコントローラー情報やリアルタイムな入力ステータスを専用コンソールに出力して監視可能にします。

---

## 2. GamePadモジュール仕様

### 2.1 複数バックエンドの優先度とフォールバック

GamePadモジュールは、以下の3つのバックエンドAPIをサポートしています。

- **XInput (優先度最高)**: Xboxコントローラーなどの標準的なコントローラーを処理します。最大4スロットまで個別プレイヤーとして判定します。
- **RawInput (フォールバック)**: WindowsのRaw Input APIを使用し、HID規格のゲームパッドから直接データを取得します。
- **DirectInput (フォールバック)**: DirectInput8を使用し、古いゲームパッドなどのレガシーデバイスから入力を取得します。

#### アクティブバックエンドの選定フロー
1. **XInput接続が検出されている場合**:
   - XInput経由で各プレイヤー（最大4名）の入力を受け付けます。アクティブなバックエンドは `GAMEPAD_INPUT_BACKEND_XINPUT` となります。
2. **XInputが未接続、またはRawInput/DirectInputに入力アクティビティがあった場合**:
   - `gamepad.cpp` 内部では、スティックの閾値（`kActivityStickThreshold = 0.12f`）やトリガーの閾値（`kActivityTriggerThreshold = 0.08f`）を超える変化（アクティビティ）を監視しています。
   - より新しい（直近で操作された）バックエンドを優先し、自動的に `GAMEPAD_INPUT_BACKEND_RAWINPUT` または `GAMEPAD_INPUT_BACKEND_DIRECTINPUT` をアクティブにします。
   - RawInputやDirectInputデバイスの入力は、XInput of プレイヤー0（スロット0）にマージ（フォールバック）されて反映されます。これにより、ゲーム側はデバイスの種類を気にせず常にプレイヤー0の入力として処理することができます。

---

### 2.2 ボタンレイアウト機能

コントローラーのボタン物理配置（A/B/X/Y）は、メーカー（Xbox仕様とNintendo Switch等の仕様）によって異なります。本モジュールでは `Gamepad_Layout` を用いて、この配置をソフト側で差し替えることができます。

```cpp
typedef enum Gamepad_Layout_tag
{
    GAMEPAD_LAYOUT_XBOX = 0,        // Xbox標準 (Aが下、Bが右、Xが左、Yが上)
    GAMEPAD_LAYOUT_SWITCH_ABXY,     // Switch/任天堂標準 (Aが右、Bが下、Xが上、Yが左)
} Gamepad_Layout;
```

`Gamepad_IsButtonDown` などの判定関数を呼ぶ際、内部で自動的に物理ボタンマスクが以下のようにマッピング変更されます。

| 抽象ボタン (`Gamepad_Button`) | XBOX レイアウト時の物理割り当て | SWITCH_ABXY レイアウト時の物理割り当て |
| :--- | :--- | :--- |
| `GPB_A` | `XINPUT_GAMEPAD_A` (下ボタン) | `XINPUT_GAMEPAD_B` (右ボタン) |
| `GPB_B` | `XINPUT_GAMEPAD_B` (右ボタン) | `XINPUT_GAMEPAD_A` (下ボタン) |
| `GPB_X` | `XINPUT_GAMEPAD_X` (左ボタン) | `XINPUT_GAMEPAD_Y` (上ボタン) |
| `GPB_Y` | `XINPUT_GAMEPAD_Y` (上ボタン) | `XINPUT_GAMEPAD_X` (左ボタン) |

---

### 2.3 主要API関数一覧

- **ライフサイクル**
  - [Gamepad_Initialize](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L43): ゲームパッドサブシステムの初期化、RawInputの登録、DirectInputの初期化を行います。
  - [Gamepad_Finalize](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L44): 割り当てられたDirectInputデバイスやRawInputリソースを解放します。
  - [Gamepad_Update](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L45): 毎フレーム呼び出され、XInput/DirectInput/RawInputの最新状態をポーリングし、アクティブなバックエンドや入力を更新します。
  - [Gamepad_ProcessMessage](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L46): Windowsのウィンドウメッセージ（`WM_INPUT` やデバイス変更通知など）を処理し、RawInputの処理を行います。

- **接続状態・情報の取得**
  - [Gamepad_IsConnected](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L48): 指定プレイヤーの接続状態を取得します。
  - [Gamepad_FindConnectedPlayer](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L49): 現在接続されている有効なプレイヤーインデックス（接続されていない場合は `-1`）を返します。
  - [Gamepad_GetConnectedMask](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L50): 接続されているプレイヤーをビットマスク形式で取得します。
  - [Gamepad_GetActiveBackend](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L63): 現在有効な入力バックエンド（`XINPUT`, `RAWINPUT`, `DIRECTINPUT`, `NONE`）を取得します。

- **入力状態の取得**
  - [Gamepad_IsButtonDown](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L51): ボタンが押されているか判定します。
  - [Gamepad_IsButtonTrigger](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L52): ボタンがこのフレームで押された瞬間か判定します。
  - [Gamepad_GetLeftStick](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L54) / [Gamepad_GetRightStick](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L55): 左右のアナログスティックの傾き値（-1.0 〜 +1.0）を取得します（デッドゾーン処理適用済み）。
  - [Gamepad_GetLeftTrigger](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L56) / [Gamepad_GetRightTrigger](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L57): 左右トリガーの押し込み量（0.0 〜 1.0）を取得します。

- **振動機能**
  - [Gamepad_SetVibration](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/gamepad.h#L59): 左右モーターの振動強さを設定します（XInputおよび対応するRawInput/DirectInputでの動作）。

---

## 3. InputManagerとの連携

`InputManager` は、ゲームプログラムが入力デバイスの違い（キーボードかゲームパッドか）を意識せずに開発できるように抽象化を行う層です。

### 3.1 アクションマッピング

ゲーム内では `Input_Action` 列挙型を介して入力を受け取ります。

```cpp
typedef enum Input_Action_tag
{
    INPUT_ACTION_DECIDE = 0,      // 決定
    INPUT_ACTION_CANCEL,          // キャンセル
    INPUT_ACTION_MENU_UP,         // メニュー上
    INPUT_ACTION_MENU_DOWN,       // メニュー下
    INPUT_ACTION_MENU_LEFT,       // メニュー左
    INPUT_ACTION_MENU_RIGHT,      // メニュー右
    INPUT_ACTION_PAUSE,           // ポーズ
} Input_Action;
```

#### マッピング仕様
- **決定 (DECIDE)**: キーボードの `ENTER` / `SPACE`、またはゲームパッドの `Aボタン` のいずれかで判定します。
- **キャンセル (CANCEL)**: キーボードの `ESCAPE` / `BACKSPACE`、またはゲームパッドの `Bボタン` のいずれかで判定します。
- **メニュー移動 (MENU_UP / DOWN / LEFT / RIGHT)**:
  - キーボードの `矢印キー`
  - ゲームパッドの `D-Pad（十字キー）`
  - ゲームパッドの `左アナログスティック` の一定以上の傾き（閾値 `0.5`）
- **一時停止 (PAUSE)**: キーボードの `Pキー`、またはゲームパッドの `STARTボタン`。

また、`Input_GetMoveVector()` を用いて、キーボード（矢印キー）と左スティックの入力強度を合成した二次元移動ベクトルを簡単に取得可能です。

---

## 4. 入力モニターコンソール (InputMonitorConsole)

`InputMonitorConsole` は、開発・デバッグを容易にするために用意された監視用のコンソールウィンドウです。

### 4.1 デバッグ動作仕様
- **ビルド構成制限**: 全ての処理は `#if defined(_DEBUG)` で囲まれており、リリースビルド時には一切コードやウィンドウが生成されません。
- **コンソールの確保**: [InputMonitorConsole_Initialize](file:///c:/Users/realryo1/Desktop/GravityNotes/framework/input_monitor_console.h#L3) 呼び出し時に `AllocConsole` を行い、標準出力・標準エラー出力をコンソール画面（`CONOUT$`）にリダイレクトします。
- **出力の間引き**: 毎フレームの出力処理は負荷がかかり、かつ文字のチラつきが発生するため、3フレームに1回のインターバルで出力が行われます。

### 4.2 出力フォーマットの解説

コンソールには以下のヘッダーが表示された後、1行で現在の情報がキャリッジリターン（`\r`）を用いてリアルタイムに上書きされ続けます。

```text
[Input Monitor] XInput priority + RawInput/DirectInput fallback
slot mask backend       A B X Y Start Back LT RT LS(x,y) RS(x,y)
```

各項目の意味は以下の通りです。

| カラム名 | 説明 |
| :--- | :--- |
| **slot** | アクティブなプレイヤーインデックス（未接続時は `-1`） |
| **mask** | 接続中のプレイヤービットマスク（例: `0x1` はプレイヤー0のみ接続） |
| **backend** | 現在入力を処理しているアクティブなAPI名 (`XInput`, `RawInput`, `DirectInput`, `None`) |
| **A B X Y** | ゲームパッドのA, B, X, Yボタンが押されているか（0: 離している、1: 押している） |
| **Start** | STARTボタンの状態（0 / 1） |
| **Back** | BACK（SELECT/SHARE）ボタンの状態（0 / 1） |
| **LT** | 左トリガーの押し込み値（0.00 〜 1.00） |
| **RT** | 右トリガーの押し込み値（0.00 〜 1.00） |
| **LS(x,y)** | 左スティックの現在値。例: `(+0.00,-1.00)` |
| **RS(x,y)** | 右スティックの現在値。例: `(+0.00,+0.00)` |

これにより、現在どのAPIがコントローラー入力を拾っているか、意図した通りのアナログ値が入っているかをゲームを実行しながら一目でデバッグ可能です。
