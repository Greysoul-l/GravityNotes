/*==============================================================================

   ポリゴン描画 [game.h]
--------------------------------------------------------------------------------

==============================================================================*/
#pragma once

#include <d3d11.h>
#include <string>
#include "define.h"

//ゲームシーンにおける定数定義
constexpr float dt = 1.0f / FPS;
constexpr float LANE_WIDTH = 1.2f;

void Game_Initialize(void);
void Game_Finalize(void);
void Game_Update(void);
void Game_Draw(void);
void Game_DebugUIDraw(void);