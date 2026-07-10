#include "game.h"
#include "define.h"
#include "sprite2d.h"
#include "texture.h"
#include "fade.h"
#include "debug_ostream.h"
#include "font.h"
#include "mouse.h"
#include "input_manager.h"
#include "model.h"
#include "debugcamera.h"
#include "debug_ui.h"
#include "sound.h"
#include "ClickFont.h"
#include "scene.h"
#include "camera.h"
#include "field.h"
#include "player.h"
#include "gamecamera.h"
#include "note_manager.h"
#include "light_game.h"
#include "status_manager.h"
#include "game_ui.h"

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pGameSprite = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;
static FontRenderer* g_pSelectedJsonText = nullptr;

static Field*         g_pField         = nullptr;
static Player*        g_pPlayer        = nullptr;
static NoteManager*   g_pNoteManager   = nullptr;
static StatusManager* g_pStatusManager = nullptr;
static bool           g_IsMouseCursorVisible = false;
static GameUI*        g_pGameUI        = nullptr;

void Game_Initialize(void)
{
	//int pad = Gamepad_FindConnectedPlayer();
	//if (pad < 0)return;//デバック時必要なし

  //各種初期化
	GameCamera::Init();
	GameLight::Init();

	g_pField = new Field();
	g_pField->Init();

	g_pStatusManager = new StatusManager();
	g_pStatusManager->Init();

	g_pNoteManager = new NoteManager();

	g_pNoteManager->Init("asset/score/" + GetPlayJson());
	//g_pNoteManager->Init("asset/score/shiningstar.json");

	g_pPlayer = new Player();
	g_pPlayer->Init(g_pNoteManager, g_pStatusManager);

	g_pGameUI = new GameUI();
	g_pGameUI->Init();

	UnLockMouse();//マウスアンロック
}

void Game_Update(void)
{
	//3D
	{
		GameCamera::Update(g_pPlayer);
		SetCameraPosition(GetCamera()->GetPos());

		g_pField->Update(g_pNoteManager->GetNoteSpeed());
		g_pPlayer->Update();
		g_pNoteManager->Update(g_pPlayer->GetLaneIndex(), g_pPlayer->GetGravityFace());

	}

	//2D描画
	{
		//③処理
		g_pGameUI->Update(g_pStatusManager);
		if (g_pStatusManager->HasNewJudge())
			g_pGameUI->NotifyJudge(g_pStatusManager->ConsumeJudge());
		//g_pChangeSceneText->Update();

		//ClickFontがクリックされた
		/*if (g_pChangeSceneText->IsClick())
		{
			SetSceneFade(SCENE_RESULT);
		}*/
	}

	//マウスカーソルを表示/非表示切り替え(デバッグ用)
	/*if (Keyboard_IsKeyDownTrigger(KK_U))
	{
		g_IsMouseCursorVisible = !g_IsMouseCursorVisible;
		Mouse_SetVisible(g_IsMouseCursorVisible);
		if (g_IsMouseCursorVisible)
		{
			UnLockMouse();
		}
		else
		{
			LockMouse();
		}
	}*/

	if (Input_IsActionTrigger(INPUT_ACTION_DEBUG_RESULT)) {
		SetSceneFade(SCENE_RESULT);
	}
}

void Game_Draw(void)
{
	//④描画
	//3D
	{
		SetDepthEnable(true);

		g_pField->Draw();
		g_pNoteManager->Draw();
		g_pPlayer->Draw();

		SetDepthEnable(false);
	}

	//2D
	{
		g_pGameUI->Draw();
		//g_pGameSprite->Draw();
		//g_pChangeSceneText->Draw();
		//g_pSelectedJsonText->Draw();
	}
}

void Game_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pGameSprite);
	SAFE_DELETE(g_pSelectedJsonText);
	SAFE_DELETE(g_pChangeSceneText);

	SAFE_DELETE(g_pField);
	SAFE_DELETE(g_pPlayer);
	if (g_pNoteManager)   { g_pNoteManager->Finalize();   SAFE_DELETE(g_pNoteManager); }
	if (g_pStatusManager) { g_pStatusManager->Finalize(); SAFE_DELETE(g_pStatusManager); }
	if (g_pGameUI)        { g_pGameUI->Finalize();        SAFE_DELETE(g_pGameUI); }
	GameLight::Finalize();
	GameCamera::Finalize();
}
