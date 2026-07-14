#pragma once

#include <string>

//リザルトに渡す構造体
struct SendResult 
{
	int maxCombo;		//最大コンボ数
	int hits;			//Hit数
	int misses;			//Miss数
	int orbgets;		//オーブ獲得
	int orblosses;		//オーブ逃し
	int fullCombo;		//フルコンボ時の総コンボ（判定）数
};

enum SCENE {
	SCENE_TITLE = 0,
	SCENE_STAGESELECT,
	SCENE_GAME,
	SCENE_RESULT,
	SCENE_DEBUG,
	SCENE_MAX,
	SCENE_NONE,
};

void Init(void);
void Update(void);
void Draw(void);
void Finalize(void);

void SetScene(SCENE id);
SCENE GetScene(void);
void SetPlayJson(const std::string& jsonName);
const std::string& GetPlayJson(void);
void SetResult(const SendResult& r);
const SendResult* GetResult();
