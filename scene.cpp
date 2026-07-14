#include "scene.h"
#include "game.h"
#include "renderer.h"
#include "keyboard.h"
#include "texture.h"
#include "title.h"
#include "stageselect.h"
#include "result.h"
#include "debugscene.h"
#include "define.h"
#include "debug_ui.h"
#include "options_manager.h"
using namespace DirectX;

static SCENE scene = SCENE_TITLE;
static std::string playjson;		//SCENE_STAGESELECTからSCENE_GAMEへのjson指定
static SendResult result = {};

void Init( void )
{
	static bool optionChecked = false;
	if (!optionChecked)
	{
		optionChecked = true;
		const std::string& directPlayPath = Options_GetDirectPlayPath();
		if (!directPlayPath.empty())
		{
			SetPlayJson(directPlayPath);
			scene = SCENE_GAME;
		}
	}

	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Initialize();
		break;
		case SCENE_STAGESELECT:
		StageSelect_Initialize();
		break;
		case SCENE_GAME:
		Game_Initialize();
		break;
		case SCENE_RESULT:
		Result_Initialize();
		break;
		case SCENE_DEBUG:
		DebugScene_Initialize();
		break;
		default:
		break;
	}
}

void Update( void )
{
	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Update();
		break;
		case SCENE_STAGESELECT:
		StageSelect_Update();
		break;
		case SCENE_GAME:
		Game_Update();
		break;
		case SCENE_RESULT:
		Result_Update();
		break;
		case SCENE_DEBUG:
		DebugScene_Update();
		break;
		default:
		break;
	}
}

void Draw( void )
{
	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Draw();
		break;
		case SCENE_STAGESELECT:
		StageSelect_Draw();
		break;
		case SCENE_GAME:
		Game_Draw();
		break;
		case SCENE_RESULT:
		Result_Draw();
		break;
		case SCENE_DEBUG:
		DebugScene_Draw();
		break;
		default:
		break;
	}

	DebugUI_Draw();
}

void Finalize( void )
{
	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Finalize();
		break;
		case SCENE_STAGESELECT:
		StageSelect_Finalize();
		break;
		case SCENE_GAME:
		Game_Finalize();
		break;
		case SCENE_RESULT:
		Result_Finalize();
		break;
		case SCENE_DEBUG:
		DebugScene_Finalize();
		break;
		default:
		break;
	}
}

void SetScene( SCENE id )
{
	Finalize();

	scene = id;

	Init();
}

SCENE GetScene( void )
{
	return scene;
}

void SetPlayJson(const std::string& jsonName)
{
	playjson = jsonName;
}

const std::string& GetPlayJson(void)
{
	return playjson;
}

void SetResult(const SendResult& r) {
	result = r;
}
const SendResult* GetResult() {
	return &result;
}
