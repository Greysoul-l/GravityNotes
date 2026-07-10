#include "result.h"
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
#include "scoresummaryloader.h"
#include "scene.h"
#include "MultiLineFontRenderer.h"
#include <sstream>
#include <iomanip>
#include "imgui/imgui.h"
#include <string>

using namespace DirectX;

enum ResultRank
{
	RR_C = 0,
	RR_B,
	RR_A,
	RR_S,
	RR_AH
};

struct ShowResult
{
	int score;
	ResultRank rank;
	int maxCombo;
	int hits;
	int misses;
	int orbgets;
	int orblosses;
};

// ①インスタンス、ポインタ用意
static Sprite2D* g_pResultBG = nullptr;
static Sprite2D* g_pResultBackUI = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;
static ScoreSummary g_ScoreSummary;
static ShowResult g_ShowResult;

static FontRenderer* g_pMusicTexts[4] = { nullptr };
static MultiLineFontRenderer* g_pAuthorFont = nullptr;
static float g_MusicTextPos[4][2];
static float g_MusicTextScale[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static Sprite2D* g_pRankTextre = nullptr;
static Sprite2D* g_pNextSceneTexture = nullptr;

// アニメーション用の構造体
struct ResultRowData {
	std::string label;		 // 左側の文字 (例: "SCORE :")
	std::string valueStr;	 // 右側の数字 (例: "12345" ※演出で文字化けする)
	int targetValue{};       // 最終的に表示する数値
	float currentX{};        // 現在のX座標
	float y{};               // Y座標
};

static const int MAX_ROWS = 5;
static ResultRowData g_ResultRows[MAX_ROWS];
static FontRenderer* g_pLabelFont = nullptr;
static FontRenderer* g_pValueFont = nullptr;

static float g_CountUpTimer = 0.0f;
static const float COUNT_UP_MAX_TIME = 90.0f;		// 90フレーム(1.5秒)でカウントアップ
static float g_ResultSceneTimer = 0.0f;
static const float ROW_DELAY = 30.0f;               // 1行ごとの遅延フレーム
static const float VALUE_START_DELAY = 30.0f;       // ラベルがすべて表示された後に数値を開始するまでの待機フレーム

// 全てのアニメーションが終了する時間
static const float RANK_ANIM_START_TIME = (MAX_ROWS * ROW_DELAY) + VALUE_START_DELAY + ((MAX_ROWS - 1) * ROW_DELAY) + COUNT_UP_MAX_TIME;
static const float RANK_ANIM_DURATION = 30.0f;		// 30フレーム(0.5秒)

void Result_Initialize(void)
{
	//======================================================================

	//プレイした楽曲の概要を取得　上書き禁止
	g_ScoreSummary = LoadSingleScoreSummary(GetPlayJson());

	////リザルトデータを実体化させてコピー　上書き禁止
	g_ShowResult.maxCombo = GetResult()->maxCombo;
	g_ShowResult.hits = GetResult()->hits;
	g_ShowResult.misses = GetResult()->misses;
	g_ShowResult.orbgets = GetResult()->orbgets;
	g_ShowResult.orblosses = GetResult()->orblosses;

	//g_ShowResult.maxCombo = 10;
	//g_ShowResult.hits = 55;
	//g_ShowResult.misses = 45;
	//g_ShowResult.orbgets = 10;
	//g_ShowResult.orblosses = 2;

	int totalNotes = g_ShowResult.hits + g_ShowResult.misses;
	if (totalNotes > 0)
	{
		g_ShowResult.score = static_cast<int>((static_cast<double>(g_ShowResult.hits) / totalNotes) * 1000000);
	}
	else
	{
		g_ShowResult.score = 0;
	}
	//hal::dout << "aaaaaaaaaaaaaaaaaaaaaa   " << g_ShowResult.score << std::endl;


	//ランク計算
	if (g_ShowResult.score >= 1000000)
	{
		g_ShowResult.rank = RR_AH;
	}
	else if (g_ShowResult.score >= 950000)
	{
		g_ShowResult.rank = RR_S;
	}
	else if (g_ShowResult.score == 900000)
	{
		g_ShowResult.rank = RR_A;
	}
	else if (g_ShowResult.score == 800000)
	{
		g_ShowResult.rank = RR_B;
	}
	else
	{
		g_ShowResult.rank = RR_C;
	}

	//======================================================================

	g_pResultBG = new Sprite2D(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },					//位置
		{ SCREEN_WIDTH, SCREEN_HEIGHT },							//サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//RGBA
		BLENDSTATE_ALFA,											//BlendState
		L"asset\\texture\\Result_BG_kari.png"						//テクスチャパス
	);

	g_pResultBackUI = new Sprite2D(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\Result_Back_UI.png"
	);

	g_pNextSceneTexture = new Sprite2D(
		{ SCREEN_WIDTH * 4 / 5, 613.0f },
		{ 85 * 4, 39 * 4 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\Result_Button_UI.png"
	);

	g_pChangeSceneText = new ClickFont(
		{ 1009, 612 },				//位置
		50.0f,														//文字サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//通常色
		{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
		"曲選択へ"													//テキスト
	);

	// フォントの生成（空文字で初期化し、Draw時にセットする）
	g_pLabelFont = new FontRenderer({ 0, 0 }, 43.0f, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, "", TA_START);
	g_pValueFont = new FontRenderer({ 0, 0 }, 43.0f, 0.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, "", TA_MIDDLE);

	// 各行の初期状態
	float startX = 100.0f;
	float startY = 294.0f;
	float lineSpacing = 60.0f; // 行間隔

	g_ResultRows[0] = { "スコア  ",    "", g_ShowResult.score, startX,  startY };
	g_ResultRows[1] = { "最大コンボ数  ", "", g_ShowResult.maxCombo, startX , startY + lineSpacing };
	g_ResultRows[2] = { "ヒット数  ",    "", g_ShowResult.hits , startX ,  startY + lineSpacing * 2 };
	g_ResultRows[3] = { "ミス数  ",  "", g_ShowResult.misses, startX ,  startY + lineSpacing * 3 };
	g_ResultRows[4] = { "オーブ数  ", "", g_ShowResult.orbgets, startX ,  startY + lineSpacing * 4 };

	// 難易度の小数点以下1桁まで表示するためのstringstreamを使用
	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << g_ScoreSummary.difficulty;


	// 楽曲情報のテキストを生成
	// [0]: 曲名, [1]: 作曲者, [2]: 譜面制作者, [3]: 難易度
	float initialPos[4][2] = {
		{ 300.0f, 152.0f },
		{ 550.0f, 220.0f },
		{ 550.0f, 250.0f },
		{ 138.0f, 613.0f }
	};

	float initialScale[4] = { 1.67f, 0.75f, 0.75f, 1.0f };

	for (int i = 0; i < 4; ++i) {
		g_MusicTextPos[i][0] = initialPos[i][0];
		g_MusicTextPos[i][1] = initialPos[i][1];
		g_MusicTextScale[i] = initialScale[i];
	}

	std::string texts[4] = {
		g_ScoreSummary.musicname,
		g_ScoreSummary.musicauthor,
		g_ScoreSummary.scoreauthor,
		"難易度 : " + ss.str()
	};

	for (int i = 0; i < 4; ++i) {
		g_pMusicTexts[i] = new FontRenderer(
			{ g_MusicTextPos[i][0], g_MusicTextPos[i][1] },
			30.0f,
			0.0f,
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			texts[i],
			TA_START
		);
		g_pMusicTexts[i]->SetSize({ g_MusicTextScale[i], g_MusicTextScale[i] });
	}

	// 作曲者、譜面制作者
	g_pAuthorFont = new MultiLineFontRenderer(
		{ 300.0f, 220.0f },
		18.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"MusicAuthor\nScoreAuthor",
		1.5f,
		TA_START
	);

	//// 達成率(accurary)によって表示するテクスチャを決定する
	std::wstring wRank = L"C"; // デフォルトはC

	switch (g_ShowResult.rank)
	{
	case RR_C:
		wRank = L"C";
		break;
	case RR_B:
		wRank = L"B";
		break;
	case RR_A:
		wRank = L"A";
		break;
	case RR_S:
		wRank = L"S";
		break;
	case RR_AH:
		wRank = L"AH";
		break;
	default:
		break;
	}

	//テクスチャのファイルパスを動的に生成
	std::wstring rankTexturePath = L"asset\\texture\\Result_Rank_" + wRank + L"_UI.png";

	g_pRankTextre = new Sprite2D(
		{ SCREEN_WIDTH / 2 + 350.0f , SCREEN_HEIGHT / 3 + 100.0f },			//位置
		{ 500, 500 },														//サイズ
		0.0f,																//回転（度）
		{ 1.0f, 1.0f, 1.0f, 0.0f },											//RGBA (初期状態は透明)
		BLENDSTATE_ALFA,													//BlendState
		rankTexturePath.c_str()												//テクスチャパス
	);

	g_CountUpTimer = 0.0f;
	g_ResultSceneTimer = 0.0f;

	UnLockMouse();//マウスアンロック
}

void Result_Update(void)
{
	//③処理
	g_pChangeSceneText->Update();

	// デバッグ用: Rキーでアニメーションをリスタート（直接インクルード削除に伴いコメントアウト）
	/*if (Keyboard_IsKeyDownTrigger(KK_R))
	{
		// 既存のタイマーリセット（g_CountUpTimerは廃止してもOK）
		g_ResultSceneTimer = 0.0f;

		// 初期座標リセット
		for (int i = 0; i < MAX_ROWS; ++i) {
			g_ResultRows[i].currentX = 100.0f; // startX
			g_ResultRows[i].valueStr = ""; // 表示リセット
		}
	}*/


	g_ResultSceneTimer += 1.0f;

	for (int i = 0; i < MAX_ROWS; ++i)
	{
		// 数値のアニメーション（ただのカウントアップ）
		float valueStartTime = (MAX_ROWS * ROW_DELAY) + VALUE_START_DELAY + (i * ROW_DELAY);
		if (g_ResultSceneTimer >= valueStartTime)
		{
			// 数値アニメーションの進行度 (0.0 ~ 1.0)
			float valueProgress = (g_ResultSceneTimer - valueStartTime) / COUNT_UP_MAX_TIME;
			if (valueProgress >= 1.0f)
			{
				valueProgress = 1.0f;
				if (i == 4) // オーブ数
				{
					int totalOrbs = g_ShowResult.orbgets + g_ShowResult.orblosses;
					g_ResultRows[i].valueStr = std::to_string(g_ResultRows[i].targetValue) + "/" + std::to_string(totalOrbs);
				}
				else
				{
					g_ResultRows[i].valueStr = std::to_string(g_ResultRows[i].targetValue);
				}
			}
			else
			{
				// イージング (Ease-Out Quad)
				float easeProgress = 1.0f - (1.0f - valueProgress) * (1.0f - valueProgress);
				int curVal = static_cast<int>(g_ResultRows[i].targetValue * easeProgress);
				if (i == 4) // オーブ数
				{
					int totalOrbs = g_ShowResult.orbgets + g_ShowResult.orblosses;
					g_ResultRows[i].valueStr = std::to_string(curVal) + "/" + std::to_string(totalOrbs);
				}
				else
				{
					g_ResultRows[i].valueStr = std::to_string(curVal);
				}
			}
		}
		else
		{
			// まだ開始時間になっていない場合は空にしておく
			g_ResultRows[i].valueStr = "";
		}
	}

	// ランクテクスチャのフェードイン＆スケールアニメーション
	if (g_ResultSceneTimer >= RANK_ANIM_START_TIME)
	{
		float progress = (g_ResultSceneTimer - RANK_ANIM_START_TIME) / RANK_ANIM_DURATION;
		if (progress > 1.0f) progress = 1.0f;
		g_pRankTextre->SetColor({ 1.0f, 1.0f, 1.0f, progress });

		// イーズイン（徐々に加速して落ちてくるスタンプ演出）
		float easeIn = progress * progress * progress;
		// 3倍(1500)から等倍(500)へ縮小
		float currentSize = 1500.0f + (500.0f - 1500.0f) * easeIn;
		g_pRankTextre->SetSize({ currentSize, currentSize });
	}
	else
	{
		g_pRankTextre->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		g_pRankTextre->SetSize({ 1500.0f, 1500.0f }); // 初期サイズは大きめ
	}

	//ClickFontがクリックされた、または決定ボタンが押された
	if (g_pChangeSceneText->IsClick() || Input_IsActionTrigger(INPUT_ACTION_DECIDE))
	{
		SetPlayJson("");//resultを抜けるときに初期化
		SetSceneFade(SCENE_STAGESELECT);
	}
}

void Result_Draw(void)
{
	//④描画
	g_pResultBG->Draw();
	g_pResultBackUI->Draw();
	g_pRankTextre->Draw();
	g_pNextSceneTexture->Draw();
	g_pChangeSceneText->Draw();
	for (int i = 0; i < 4; ++i) {
		if (g_pMusicTexts[i]) g_pMusicTexts[i]->Draw();
	}


	if (g_pLabelFont && g_pValueFont)
	{
		for (int i = 0; i < MAX_ROWS; ++i)
		{
			float labelStartTime = i * ROW_DELAY;

			// ラベルの開始時間を過ぎていたら描画
			if (g_ResultSceneTimer >= labelStartTime) {
				g_pLabelFont->SetPos({ g_ResultRows[i].currentX + (i * 20.0f), g_ResultRows[i].y });
				g_pLabelFont->SetText(g_ResultRows[i].label);
				g_pLabelFont->Draw();
			}

			// valueStr が空でなければ数値を描画（Update側で制御済み）
			if (!g_ResultRows[i].valueStr.empty()) {
				g_pValueFont->SetPos({ g_ResultRows[i].currentX + 350.0f + (i * 20.0f), g_ResultRows[i].y });
				g_pValueFont->SetText(g_ResultRows[i].valueStr);
				g_pValueFont->Draw();
			}
		}
	}

	g_pAuthorFont->Draw();
}

void Result_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pResultBG);
	SAFE_DELETE(g_pResultBackUI);
	SAFE_DELETE(g_pChangeSceneText);
	for (int i = 0; i < 4; ++i) {
		SAFE_DELETE(g_pMusicTexts[i]);
	}

	SAFE_DELETE(g_pRankTextre);
	SAFE_DELETE(g_pLabelFont);
	SAFE_DELETE(g_pValueFont);
	SAFE_DELETE(g_pAuthorFont);
	SAFE_DELETE(g_pRankTextre);
	SAFE_DELETE(g_pNextSceneTexture);

}

void Result_DebugUIDraw(void)
{
	ImGui::Begin("Result Scene Editor");
	if (ImGui::CollapsingHeader("Music Texts")) {

	}
	if (ImGui::CollapsingHeader("Change Scene Text")) {
		if (g_pChangeSceneText) {
			float pos[2] = { g_pChangeSceneText->GetPosX(), g_pChangeSceneText->GetPosY() };
			if (ImGui::DragFloat2("Position##ChangeSceneText", pos, 1.0f)) {
				g_pChangeSceneText->SetPos({ pos[0], pos[1] });
			}
		}
	}
	ImGui::End();
}
