#include "title.h"
#include "sprite2d.h"
#include "texture.h"
#include "input_manager.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "ClickFont.h"
#include "Movie.h"

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pTitleSprite = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;
static ClickFont* g_pDebugSceneText = nullptr;
static Movie* g_pTitleMovie;


void Title_Initialize(void)
{
	g_pTitleMovie = new Movie(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },					//位置
		{ SCREEN_WIDTH },											//サイズ
		0.0f,														//回転（度）
		{ 1.0f,1.0f,1.0f, 1.0f },
		BLENDSTATE_NONE,
		L"asset\\movie\\titlemovie_roop.mp4"
	);

	g_pChangeSceneText = new ClickFont(
		{ SCREEN_WIDTH / 3.0f, 600.0f },			//位置
		50.0f,														//文字サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//通常色
		{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
		"Game Start"										//テキスト
	);

	g_pDebugSceneText = new ClickFont(
		{ SCREEN_WIDTH - 100.0f, 50.0f },			//位置
		20.0f,														//文字サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.5f },									//通常色
		{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
		"[debug] "										//テキスト
	);

	UnLockMouse();//マウスアンロック
}



void Title_Update(void)
{
	//③処理
	g_pTitleMovie->Update();
	g_pChangeSceneText->Update();
	g_pDebugSceneText->Update();

	//ClickFontがクリックされた、または決定ボタンが押された
	if (g_pChangeSceneText->IsClick() || Input_IsActionTrigger(INPUT_ACTION_DECIDE))
	{
		SetSceneFade(SCENE_STAGESELECT);
	}
	if (g_pDebugSceneText->IsClick())
	{
		SetSceneFade(SCENE_DEBUG);
	}

}

void Title_Draw(void)
{
	//④描画
	g_pTitleMovie->Draw();
	g_pChangeSceneText->Draw();
	g_pDebugSceneText->Draw();

}

void Title_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pTitleMovie);
	SAFE_DELETE(g_pChangeSceneText);
	SAFE_DELETE(g_pDebugSceneText);
}
