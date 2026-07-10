#pragma once

#include "gamepad.h"

typedef enum Input_Action_tag
{
    INPUT_ACTION_DECIDE = 0,        // 決定 (Space, Enter / Aボタン)
    INPUT_ACTION_CANCEL,            // キャンセル / 戻る (BackSpace / Bボタン)
    INPUT_ACTION_MENU_UP,           // メニュー上移動 (W, UP, DPad-UP, LStick-UP)
    INPUT_ACTION_MENU_DOWN,         // メニュー下移動 (S, DOWN, DPad-DOWN, LStick-DOWN)
    INPUT_ACTION_MENU_LEFT,         // メニュー左移動 (A, LEFT, DPad-LEFT, LStick-LEFT)
    INPUT_ACTION_MENU_RIGHT,        // メニュー右移動 (D, RIGHT, DPad-RIGHT, LStick-RIGHT)
    INPUT_ACTION_PAUSE,             // ポーズ画面 (Escape / STARTボタン)
    INPUT_ACTION_ATTACK,            // 攻撃 / ノーツタップ (Space / LT, RT, LB, RB)
    INPUT_ACTION_MOVE_UP,           // プレイヤー移動上 (W, DPad-UP, LStick-UP)
    INPUT_ACTION_MOVE_DOWN,         // プレイヤー移動下 (S, DPad-DOWN, LStick-DOWN)
    INPUT_ACTION_MOVE_LEFT,         // プレイヤー移動左 (A, DPad-LEFT, LStick-LEFT)
    INPUT_ACTION_MOVE_RIGHT,        // プレイヤー移動右 (D, DPad-RIGHT, LStick-RIGHT)
    INPUT_ACTION_GRAVITY_UP,        // 重力移動天井 (UP, RStick-UP)
    INPUT_ACTION_GRAVITY_DOWN,      // 重力移動床 (DOWN, RStick-DOWN)
    INPUT_ACTION_GRAVITY_LEFT,      // 重力移動左壁 (LEFT, RStick-LEFT)
    INPUT_ACTION_GRAVITY_RIGHT,     // 重力移動右壁 (RIGHT, RStick-RIGHT)
    INPUT_ACTION_DEBUG_RESULT,      // デバッグ用強制リザルト遷移 (F1キー)
} Input_Action;

typedef struct Input_Vector2_tag
{
    float x;
    float y;
} Input_Vector2;

void Input_Initialize(void);
void Input_Finalize(void);
void Input_Update(void);

bool Input_IsActionDown(Input_Action action);
bool Input_IsActionTrigger(Input_Action action);

Input_Vector2 Input_GetMoveVector(void);
Input_Vector2 Input_GetLookVector(void);

void Input_SetRumble(float leftMotor, float rightMotor);

void Input_SetGamepadLayout(Gamepad_Layout layout);
Gamepad_Layout Input_GetGamepadLayout(void);