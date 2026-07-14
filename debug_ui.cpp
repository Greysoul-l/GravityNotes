#include "debug_ui.h"
#include "debug_params.h"
#include "../framework/imgui/imgui.h"
#include "../framework/imgui/imgui_impl_dx11.h"
#include "../framework/imgui/imgui_impl_win32.h"
#include "../framework/keyboard.h"
#include "../framework/camera.h"
#include "gamepad.h"
#include "scene.h"
#include "result.h"
#include "light_game.h"
#include "game_ui.h"
#include "game.h"

static bool s_IsOpen = true;

void DebugUI_Draw()
{
#ifdef _DEBUG
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

	/*ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin("UI",&s_IsOpen);
	{
		
	}
	ImGui::End();*/

	GameLight::Draw();

	if (GetScene() == SCENE_RESULT)
	{
		Result_DebugUIDraw();
	}

	if (GetScene() == SCENE_GAME)
	{
		Game_DebugUIDraw();
	}

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
}

bool DebugUI_IsOpen()
{
    return s_IsOpen;
}
