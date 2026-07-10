#include "debugrigplayer.h"
#include "light.h"
#include "sprite2d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "renderer.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "sprite3d.h"
#include "anim_sprite3d.h"
#include "ClickFont.h"
#include "camera.h"
#include "movie.h"
#include "debugcamera.h"
#include "../framework/imgui/imgui.h"
#include "../framework/imgui/imgui_impl_dx11.h"
#include "../framework/imgui/imgui_impl_win32.h"
#include <string>
#include <cmath>
#include "split_bilboard.h"

using namespace DirectX;

static AnimSprite3D* g_pRigPlayer = nullptr;
static Sprite3D* g_pFieldNormal = nullptr;
static Sprite3D* g_pBox1x1x1 = nullptr;
static PointLight* g_pMainLight = nullptr;
static AmbientLight g_AmbientLight(XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
static FontRenderer* g_pGuideFont = nullptr;
static FontRenderer* g_pAnimInfoFont = nullptr;
static SplitBilBoard* g_pEffectSlash = nullptr;
static bool g_IsEffectSlashActive = false;
static bool g_IsMouseCursorVisible = false;
static bool g_IsOverridePlaying = false;
static XMFLOAT4 g_LightPosition = { 0.0f, 3.0f, 0.0f, 1.0f };
static float g_LightRange = 100.0f;
static float g_LightIntensity = 1.0f;
static XMFLOAT4 g_AmbientColor = { 0.4f, 0.4f, 0.4f, 1.0f };

static int GetTriggeredAnimationSlot()
{
	static const Keyboard_Keys keys[] = {
		KK_D1,
		KK_D2,
		KK_D3,
		KK_D4,
		KK_D5,
		KK_D6,
		KK_D7,
		KK_D8,
		KK_D9,
		KK_D0,
	};

	for (int i = 0; i < (int)ARRAYSIZE(keys); i++)
	{
		if (Keyboard_IsKeyDownTrigger(keys[i]))
		{
			return i;
		}
	}

	return -1;
}

void DebugRigPlayer_Initialize(void)
{
	g_pRigPlayer = new AnimSprite3D(
		{ 0.0f, 0.0f, 0.0f },
		{ 0.01f, 0.01f, 0.01f },
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\knight_02.fbx",
		S_PBR
	);

	g_pFieldNormal = new Sprite3D(
		{ 0.0f, 2.0f, 0.0f },
		{ 4.0f, 4.0f, 4.0f },	
		{ 0.0f, 0.0f, 0.0f },
		"asset\\model\\field_allnormal.fbx",
		S_PHONG
	);

	//g_pBox1x1x1 = new Sprite3D(
	//	{ 1.0f, 0.0f, 0.0f },
	//	{ 1.0f, 1.0f, 1.0f },
	//	{ 0.0f, 0.0f, 0.0f },
	//	"asset\\model\\cube.fbx",
	//	S_LAMBERT
	//);

	if (g_pRigPlayer)
	{
		g_pRigPlayer->SetAnimationBlendDuration(0.2);
		// 初期状態で "run" アニメーションをベース、"attack" アニメーションを右肩と武器ボーンから上書き
		if (g_pRigPlayer->PlayAnimationByName("run", true))
		{
			std::vector<std::string> overrideBones = { "BJnt_R_shoulder", "BJnt_sword" };
			g_pRigPlayer->PlayOverrideAnimation("attack", overrideBones, true);
			// フォントの初期化は後で行われるが、ここでは再生成功したため後続のテキスト設定で上書きする
		}
		else
		{
			unsigned int animCount = g_pRigPlayer->GetAnimationCount();
			if (animCount > 0)
			{
				g_pRigPlayer->PlayAnimationByIndex(0, true);
			}
		}
		g_pRigPlayer->UpdateAnimation(0.0f);
	}

	g_pMainLight = new PointLight(
		TRUE,
		g_LightPosition,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		g_LightRange,
		g_LightIntensity
	);
	g_pMainLight->Apply(g_AmbientLight);

	DebugCamera_Initialize();
	if (GetCamera())
	{
		GetCamera()->SetTargetPos({ 0.0f, -3.0f, 5.0f });
		SetCameraPosition(GetCamera()->GetPos());
	}

	g_pGuideFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, 30.0f },
		22.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"WASD/SPACE/SHIFT:Camera  1-0:Animation"
	);
	g_pGuideFont->PreCacheGlyphs();

	g_pAnimInfoFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 40.0f },
		22.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		(g_pRigPlayer && g_pRigPlayer->IsOverrideAnimationActive()) ? 
			"Animation: run (Override: attack @ BJnt_R_shoulder & BJnt_sword)" : "Animation: none"
	);
	g_pAnimInfoFont->PreCacheGlyphs();

	g_pEffectSlash = new SplitBilBoard(
		4, 4,
		{ 0.0f, 0.5f, 0.0f },
		{ 2.0f, 2.0f },
		{ 90.0f, 0.0f, 0.0f },
		"asset\\texture\\effect_slash_ver01.png",
		true
	);
	if (g_pEffectSlash)
	{
		g_pEffectSlash->SetLoop(false);
		g_pEffectSlash->SetFPS(30.0f);
		g_pEffectSlash->SetAnimationEnabled(false);
		g_pEffectSlash->SetBillboardMode(false);
	}
}

void DebugRigPlayer_Update(void)
{
	DebugCamera_Update();

	if (Keyboard_IsKeyDownTrigger(KK_U))
	{
		g_IsMouseCursorVisible = !g_IsMouseCursorVisible;
		if (g_IsMouseCursorVisible)
		{
			UnLockMouse();
		}
		else
		{
			LockMouse();
		}
	}


	if (GetCamera())
	{
		GetCamera()->SetTargetPos({ 0.0f, -3.0f, 5.0f });
		SetCameraPosition(GetCamera()->GetPos());
	}

	if (g_pMainLight)
	{
		g_pMainLight->SetPosition(g_LightPosition);
		g_pMainLight->SetRange(g_LightRange);
		g_pMainLight->SetIntensity(g_LightIntensity);
		g_AmbientLight.SetColor(g_AmbientColor);
		g_pMainLight->Apply(g_AmbientLight);
	}

	if (g_pRigPlayer)
	{
		int animSlot = GetTriggeredAnimationSlot();
		unsigned int animCount = g_pRigPlayer->GetAnimationCount();
		if (animSlot >= 0)
		{
			// キー0 (スロット9) が押された場合は部分アニメーション上書きを設定
			if (animSlot == 9)
			{
				if (g_pRigPlayer->PlayAnimationByName("run", true))
				{
					std::vector<std::string> overrideBones = { "BJnt_R_shoulder", "BJnt_sword" };
					g_pRigPlayer->PlayOverrideAnimation("attack", overrideBones, false);
					g_IsOverridePlaying = true;
					if (g_pAnimInfoFont)
					{
						g_pAnimInfoFont->SetText("Animation: run (Override: attack @ BJnt_R_shoulder & BJnt_sword)");
						g_pAnimInfoFont->PreCacheGlyphs();
					}
					if (g_pEffectSlash)
					{
						g_pEffectSlash->SetTextureIndex(0);
						g_pEffectSlash->SetAnimationEnabled(true);
						g_IsEffectSlashActive = true;
					}
				}
			}
			else if ((unsigned int)animSlot < animCount)
			{
				// 通常のアニメーション切り替え時はオーバーライドを停止
				g_pRigPlayer->StopOverrideAnimation();
				g_IsOverridePlaying = false;
				if (g_pRigPlayer->PlayAnimationByIndex((unsigned int)animSlot, true))
				{
					g_pRigPlayer->UpdateAnimation(0.0f);
					const char* animName = g_pRigPlayer->GetAnimationName((unsigned int)animSlot);
					if (g_pAnimInfoFont)
					{
						std::string text = "Animation: ";
						text += (animName && animName[0] != '\0') ? animName : "<noname>";
						g_pAnimInfoFont->SetText(text);
						g_pAnimInfoFont->PreCacheGlyphs();
					}
				}
			}
		}

		if (g_pAnimInfoFont && animCount == 0)
		{
			g_pAnimInfoFont->SetText("Animation: not found");
		}

		g_pRigPlayer->UpdateAnimation(1.0f / 60.0f);

		if (g_IsOverridePlaying && !g_pRigPlayer->IsOverrideAnimationActive())
		{
			g_pRigPlayer->StopOverrideAnimation();
			g_IsOverridePlaying = false;
			if (g_pAnimInfoFont)
			{
				g_pAnimInfoFont->SetText("Animation: run");
				g_pAnimInfoFont->PreCacheGlyphs();
			}
		}
	}


	if (g_pEffectSlash && g_IsEffectSlashActive)
	{
		g_pEffectSlash->Update();
		if (!g_pEffectSlash->IsAnimationEnabled())
		{
			g_IsEffectSlashActive = false;
		}
	}
}

void DebugRigPlayer_Draw(void)
{
	SetDepthEnable(true);
	if (g_pRigPlayer)
	{
		g_pRigPlayer->Draw();
	}
	if (g_pFieldNormal)
	{
		g_pFieldNormal->Draw();
	}

	if (g_pBox1x1x1)
	{
		g_pBox1x1x1->Draw();
	}

	if (g_pEffectSlash && g_IsEffectSlashActive)
	{
		g_pEffectSlash->Draw();
	}

	SetDepthEnable(false);
	Sprite_BeginDraw2D();
	if (g_pGuideFont)
	{
		g_pGuideFont->Draw();
	}
	if (g_pAnimInfoFont)
	{
		g_pAnimInfoFont->Draw();
	}
	DebugCamera_Draw();
	Sprite_EndDraw2D();

#ifdef _DEBUG
	if (ImGui::GetCurrentContext())
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Rig PBR");
		ImGui::DragFloat3("Light Pos", &g_LightPosition.x, 0.1f, -20.0f, 20.0f);
		ImGui::SliderFloat("Light Range", &g_LightRange, 1.0f, 200.0f);
		ImGui::SliderFloat("Light Intensity", &g_LightIntensity, 0.0f, 5.0f);
		ImGui::ColorEdit3("Ambient", &g_AmbientColor.x);
		ImGui::End();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
#endif
}

void DebugRigPlayer_Finalize(void)
{
	SAFE_DELETE(g_pRigPlayer);
	SAFE_DELETE(g_pFieldNormal);
	SAFE_DELETE(g_pMainLight);
	SAFE_DELETE(g_pGuideFont);
	SAFE_DELETE(g_pAnimInfoFont);
	SAFE_DELETE(g_pBox1x1x1);
	SAFE_DELETE(g_pEffectSlash);
	DebugCamera_Finalize();
}
