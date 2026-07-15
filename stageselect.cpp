#include "stageselect.h"
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
#include "MultiLineFontRenderer.h"
#include "scoresummaryloader.h"
#include "scene.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>

using namespace DirectX;

// ==========================================
// 状態定義およびグローバル変数
// ==========================================

// 剣＆盾のステージ選択画面における一連のアクションを管理するステートマシン
enum SwordShieldState {
	STATE_PLAYING,          // 安定状態：盾が等速回転し、剣が盾に刺さっている状態
	STATE_PULLING_SWORD,    // プレイヤーがステージを変更：剣が盾から真上に引き抜かれ、盾にブレーキがかかる状態
	STATE_CHANGING_SHIELD,  // 剣が完全に引き抜かれ、盾が停止：1フレームで盾を切り替える処理を行う状態
	STATE_PLUNGING_SWORD    // 盾の切り替え完了：剣が新しい盾に向かってゆっくりと突き刺さる状態
};

static SwordShieldState g_CurrentState = STATE_PLAYING; // 初期状態

// 左側のレコードアルバムに対応するステージ数/曲数を管理
static int g_MaxStages = 0;
static int g_SelectedStage = 0;                  // 現在再生中のステージ
static int g_NextStage = 0;                      // 次のステージ（遷移完了待ち）

// グラフィックオブジェクト（スプライト＆フォントポインタ）
static Sprite2D* g_pResultBG = nullptr;           // 最背面の背景画像
static Sprite2D* g_pBackground = nullptr;         // レコードプレーヤーの背景画像
static Sprite2D* g_pMainShield = nullptr;         // 中央のメイン回転盾（以前の g_pMainVinyl）
static Sprite2D* g_pShieldFrame = nullptr;        // メイン盾の外枠フレーム（以前の g_pRecordFrame）
static Sprite2D* g_pSword = nullptr;              // 盾に刺さる剣（以前の g_pToneArm）
static std::vector<ClickSprite2D*> g_pStageShields;  // 左側に並ぶ小さな盾の列（以前の g_pStageDisks）
static Sprite2D* g_pStartGameBG = nullptr;         // ゲーム開始ボタン背景
static ClickFont* g_pStartGameText = nullptr;      // ゲーム開始テキスト

// アニメーション制御変数
static float g_ShieldRotation = 0.0f;             // メイン盾の現在の回転角度（度数法、以前の g_VinylRotation）
static float g_SwordLiftOffset = 0.0f;            // 剣の引き上げ量（0 = 盾に刺さっている状態、SWORD_LIFT_DISTANCE = 完全に引き抜かれた状態、以前の g_ToneArmAngle）
static float g_ShieldSpeed = 0.5f;                // メイン盾の現在の回転速度（スムーズな減速用、以前の g_DiscSpeed）
const float SWORD_LIFT_DISTANCE = 360.0f;         // 盾変更時に剣が真上に引き上げられるピクセル数
static float g_SwordBaseX = 0.0f;                 // 剣の基準X座標（盾の中心軸に合わせる）
static float g_SwordBaseY = 0.0f;                 // 剣が盾に完全に刺さっているときの基準Y座標（最も低い位置）


const float SWORD_DIAGONAL_FACTOR = 0.70710678f;  // cos(45°) = sin(45°): 剣を垂直ではなく45度の斜め方向（右上 ↘ 左下）に突き刺すための係数

static float g_ScrollOffset = 0.0f;      // 現在のオフセット（滑らかに補間するためのfloat）
static float g_ScrollTarget = 0.0f;      // 目標のオフセット

// JSONからの譜面データの管理とスコア表示（日本語コード部分より）
static MultiLineFontRenderer* g_pScoreInfoText = nullptr;
static std::vector<ScoreSummary> g_ScoreSummaries;
static int g_SelectedScoreIndex = 0;

static SoundData* g_pCurrentBgmData = nullptr;    // 現在再生中の楽曲 of サウンドデータポインタ
static std::string g_LoadedBgmPath = "";          // 重複ロードを避けるための現在ロード中のサウンドファイルパス


// 小さな盾の外枠用スプライトの宣言
static Sprite2D* g_pSmallShieldFrame = nullptr;     // <--- 新規追加: 小さな盾の外枠フレーム


// ==========================================
// ヘルパー関数
// ==========================================

// UTF-8からワイド文字列への変換
static std::wstring Utf8ToWide(const std::string& utf8Str)
{
	if (utf8Str.empty()) return L"";
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	if (size_needed <= 0) return L"";
	std::wstring wstr(size_needed - 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wstr[0], size_needed);
	return wstr;
}

// ファイル存在確認ヘルパー
static bool FileExists(const std::string& path)
{
	std::ifstream f(path.c_str());
	return f.good();
}

// サムネイル画像のパス解決（存在しない場合はデフォルト）
static std::wstring GetThumbnailPath(const std::string& thumbnail)
{
	std::string path = "asset\\score\\" + thumbnail;
	if (thumbnail.empty() || !FileExists(path)) {
		path = "asset\\texture\\notfound_thumbnail.png";
	}
	return Utf8ToWide(path);
}

// 現在選択されているJSONファイル名を取得
static std::string GetSelectedJsonName()
{
	if (g_ScoreSummaries.empty()) return "";

	if (g_SelectedScoreIndex < 0) g_SelectedScoreIndex = 0;
	if (g_SelectedScoreIndex >= static_cast<int>(g_ScoreSummaries.size())) {
		g_SelectedScoreIndex = static_cast<int>(g_ScoreSummaries.size()) - 1;
	}
	return g_ScoreSummaries[static_cast<size_t>(g_SelectedScoreIndex)].jsonname;
}

static void UpdateBgmFromSelection()
{
	if (g_ScoreSummaries.empty()) return;

	// 選択された曲のインデックスが範囲内であることを保証
	if (g_SelectedScoreIndex < 0) g_SelectedScoreIndex = 0;
	if (g_SelectedScoreIndex >= static_cast<int>(g_ScoreSummaries.size())) {
		g_SelectedScoreIndex = static_cast<int>(g_ScoreSummaries.size()) - 1;
	}

	const ScoreSummary& summary = g_ScoreSummaries[g_SelectedScoreIndex];
	std::string soundPath = ResolveMusicPath(summary.music);

	// 選択された曲が現在再生中の曲と同じ場合はそのまま
	if (g_LoadedBgmPath == soundPath) return;

	// RAMのオーバーフローを防ぐため、古い曲を停止する（解放はシーン遷移時のキャッシュ一括クリアに任せる）
	if (g_pCurrentBgmData != nullptr) {
		StopSound(g_pCurrentBgmData);
		// UnloadSound(g_pCurrentBgmData);
		g_pCurrentBgmData = nullptr;
	}

	// 物理ディレクトリから .mp3 ファイルを RAM にロードして再生
	g_pCurrentBgmData = LoadMP3(soundPath);
	if (g_pCurrentBgmData != nullptr) {
		PlaySound(g_pCurrentBgmData, true); // ループ再生
		g_LoadedBgmPath = soundPath;
	}
	else {
		g_LoadedBgmPath = "";
	}
}

// 楽曲の詳細情報を表示するテキストを更新
static void RefreshSelectedScoreText()
{
	if (g_pScoreInfoText == nullptr) return;

	if (g_ScoreSummaries.empty()) {
		g_pScoreInfoText->SetText("No score json found");
		return;
	}

	if (g_SelectedScoreIndex < 0) g_SelectedScoreIndex = 0;
	if (g_SelectedScoreIndex >= static_cast<int>(g_ScoreSummaries.size())) {
		g_SelectedScoreIndex = static_cast<int>(g_ScoreSummaries.size()) - 1;
	}

	const ScoreSummary& summary = g_ScoreSummaries[static_cast<size_t>(g_SelectedScoreIndex)];

	char buf[1024] = {};
	std::snprintf(
		buf,
		sizeof(buf),
		"[%d/%d]\nMusic: %s\nComposer: %s\nCharter: %s\nDifficulty: %.1f\nBPM: %.1f\nJSON: %s",
		g_SelectedScoreIndex + 1,
		static_cast<int>(g_ScoreSummaries.size()),
		summary.musicname.c_str(),
		summary.musicauthor.c_str(),
		summary.scoreauthor.c_str(),
		summary.difficulty,
		summary.bpm,
		summary.jsonname.c_str()
	);
	g_pScoreInfoText->SetText(buf);
}

// 左右矢印キーが押されたときに選択されている曲のインデックスを変更
static void ChangeSelectedScore(int delta)
{
	if (g_ScoreSummaries.empty()) return;

	const int count = static_cast<int>(g_ScoreSummaries.size());
	g_SelectedScoreIndex = (g_SelectedScoreIndex + delta) % count;
	if (g_SelectedScoreIndex < 0) {
		g_SelectedScoreIndex += count;
	}

	RefreshSelectedScoreText();
}

// ==========================================
// 初期化関数 (INITIALIZE)
// ==========================================
void StageSelect_Initialize(void)
{
	// BGMデータ管理用変数の初期化
	g_pCurrentBgmData = nullptr;
	g_LoadedBgmPath = "";

	// JSONシステムから楽曲リストをロード
	g_ScoreSummaries = LoadScoreSummaries();
	g_MaxStages = static_cast<int>(g_ScoreSummaries.size());

	const bool loaded = !g_ScoreSummaries.empty();
	hal::dout << "[StageSelect] Score summary reload: "
		<< (loaded ? "SUCCESS" : "FAILED")
		<< " Count=" << g_ScoreSummaries.size()
		<< std::endl;

	// BGMのプリロードを行う（選曲切り替え時のフリーズを防止するため、キャッシュシステムに事前に登録しておく）
	for (const auto& summary : g_ScoreSummaries) {
		if (!summary.music.empty()) {
			std::string soundPath = ResolveMusicPath(summary.music);
			LoadMP3(soundPath);
		}
	}

	g_SelectedStage = 0;

	g_pResultBG = new Sprite2D(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ SCREEN_WIDTH, SCREEN_HEIGHT },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\Result_BG_kari.png"
	);

	// 1. 背景画像の初期化（テクスチャサイズに合わせて調整）
	g_pBackground = new Sprite2D(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		{ 954.0f, 717.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\a.png"
	);

	// 2. 左隅に縦に並ぶ小さなディスクの列を初期化
	g_pStageShields.resize(g_MaxStages, nullptr);
	for (int i = 0; i < g_MaxStages; i++) {
		float posX = (SCREEN_WIDTH / 2.0f) + 550.0f;
		float posY = 70.0f + (i * 150.0f); // 各ディスクを縦方向に等間隔で配置

		std::wstring thumbPath = GetThumbnailPath(g_ScoreSummaries[i].thumbnail);

		g_pStageShields[i] = new ClickSprite2D(
			{ posX, posY },
			{ 75.0f, 75.0f },
			0.0f,
			{ 1.0f, 1.0f, 1.0f, 1.0f },
			BLENDSTATE_ALFA,
			thumbPath.c_str()
		);
	}

	// 3. 背景のターンテーブルにぴったり収まるメインディスクを初期化
	std::wstring mainThumbPath = L"";
	if (g_MaxStages > 0) {
		mainThumbPath = GetThumbnailPath(g_ScoreSummaries[g_SelectedStage].thumbnail);
	}
	else {
		mainThumbPath = L"asset\\texture\\notfound_thumbnail.png";
	}
	g_pMainShield = new Sprite2D(
		{ (SCREEN_WIDTH / 2.0f) - 30.0f , (SCREEN_HEIGHT / 2.0f) + 10.0f },
		{ 250.0f, 250.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		mainThumbPath.c_str()
	);

	g_pShieldFrame = new Sprite2D(
		{ (SCREEN_WIDTH / 2.0f) - 30.0f , (SCREEN_HEIGHT / 2.0f) + 10.0f },
		{ 650.0f, 650.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\Untitled-1.png"
	);

	// 4. メイン盾の中心に合わせて、剣の初期位置を設定（上からまっすぐ突き刺さるように配置）
	g_SwordBaseX = (SCREEN_WIDTH / 2.0f) + 10.0f;       // 盾の中心と同じX軸
	g_SwordBaseY = 300.0f;       // 剣が盾に完全に刺さっているときのY座標（最も低い位置）
	g_pSword = new Sprite2D(
		{ g_SwordBaseX, g_SwordBaseY },
		{ 800.0f, 800.0f },
		45.0f, // 傾き回転はさせず、剣は常に直立状態にする
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\Untitled-3.png" // TODO: アセットが用意できたら本物の剣の画像に差し替える
	);

	// 5. 曲情報/スコア表示クラスを初期化（画面右側に配置）
	g_pScoreInfoText = new MultiLineFontRenderer(
		{ SCREEN_WIDTH - 150.0f, SCREEN_HEIGHT - 400.0f }, // 画面外にはみ出さないように位置を調整
		28.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"Loading...",
		1.35f
	);

	// StageSelect_Initialize の最後（UnLockMouseの手前）にこの処理を追加
	g_pSmallShieldFrame = new Sprite2D(
		{ 0.0f, 0.0f },          // 初期位置は仮として(0,0)に設定（描画時に動的に更新される）
		{ 200.0f, 200.0f },      // サイズ（外枠として囲うため、サムネイル画像サイズ110.0fより少し大きめに設定）
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\Untitled-1.png" // <--- 小さな盾の外枠画像のパス
	);
	// ディスク0の最初のBGMを自動的に検索して再生
	RefreshSelectedScoreText();
	UpdateBgmFromSelection();

	// 物理挙動に関する初期状態のパラメータを再設定
	g_CurrentState = STATE_PLAYING;
	g_SwordLiftOffset = 0.0f; // 剣は初期状態で完全に盾に刺さっている
	g_ShieldSpeed = 0.5f;

	g_pStartGameBG = new Sprite2D(
		{ 1068.0f, 612.0f},
		{ 85 * 4, 39 * 4 },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_ALFA,
		L"asset\\texture\\Result_Button_UI.png"
	);

	g_pStartGameText = new ClickFont(
		{ 1051.0f, 611.0f },				//位置
		50.0f,														//文字サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//通常色
		{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
		"ゲーム開始"													//テキスト
	);

	UnLockMouse(); // ユーザー操作のためにマウスのロックを解除
}

// ==========================================
// ロジック更新関数 (UPDATE)
// ==========================================
void StageSelect_Update(void)
{
	// パーセンテージによる線形補間（lerp）の代わりに一定速度で移動する。
	// 理由：%によるlerp（offset += (target-offset)*0.15f）は目標に近づくほど指数関数的に遅くなる。
	// そのため、リストの端にあるディスク（オフセットが境界である0またはMAX_STAGESに近づくとき）は、
	// 境界付近で数十フレームの間「カクつく/静止する」状態になり、最終的に閾値に達したときに突然ジャンプ（スナップ）してしまう。
	// 固定の移動ステップを使用することで、一定 of フレーム数で常に正確に目標値（target）に到達し、
	// 均等な動きになり、カクつきの後にジャンプする現象が発生しなくなる。
	const float SCROLL_STEP = 1.0f / 25.0f; // 約25フレームで1段階完了（トーンアームの昇降時間と同期）
	if (g_ScrollOffset < g_ScrollTarget) {
		g_ScrollOffset += SCROLL_STEP;
		if (g_ScrollOffset > g_ScrollTarget) g_ScrollOffset = g_ScrollTarget;
	}
	else if (g_ScrollOffset > g_ScrollTarget) {
		g_ScrollOffset -= SCROLL_STEP;
		if (g_ScrollOffset < g_ScrollTarget) g_ScrollOffset = g_ScrollTarget;
	}

	float anchorY = 80.0f;
	float spacing = 150.0f;
	// --- パート 1: キーボード/マウスの入力制御 ---
	// ディスクが安定して回転している状態（STATE_PLAYING）でのみ曲変更コマンドを受け付ける
	if (g_CurrentState == STATE_PLAYING)
	{
		bool isInputPressed = false;

		// 上下矢印、wasd、DPad、Lスティック上下でレコードのステージを変更
		if (Input_IsActionTrigger(INPUT_ACTION_MENU_UP)) {
			g_NextStage = g_SelectedStage - 1;
			if (g_NextStage < 0) g_NextStage = g_MaxStages - 1;
			isInputPressed = true;
			//ChangeSelectedScore(-1);
			g_ScrollTarget -= 1.0f;  // 1段階上へスライド
		}
		else if (Input_IsActionTrigger(INPUT_ACTION_MENU_DOWN)) {
			g_NextStage = g_SelectedStage + 1;
			if (g_NextStage >= g_MaxStages) g_NextStage = 0;
			isInputPressed = true;
			//ChangeSelectedScore(1);
			g_ScrollTarget += 1.0f;  // 1段階下へスライド
		}

		//// 左右矢印キーでJSONデータリスト内の楽曲を変更
		//if (Keyboard_IsKeyDownTrigger(KK_LEFT)) {
		//	ChangeSelectedScore(-1);
		//}
		//if (Keyboard_IsKeyDownTrigger(KK_RIGHT)) {
		//	ChangeSelectedScore(1);
		//}

		// ステージ（盾）の変更操作があった場合、剣を引き抜く一連のアクションを開始する
		if (isInputPressed) {
			g_CurrentState = STATE_PULLING_SWORD;
			if (g_pCurrentBgmData != nullptr) {
				StopSound(g_pCurrentBgmData);
				// UnloadSound(g_pCurrentBgmData);
				g_pCurrentBgmData = nullptr;
			}
			g_LoadedBgmPath = "";
		}
	}

	// --- パート 2: 状態遷移（ステートマシン）の処理 ---
	switch (g_CurrentState)
	{
	case STATE_PLAYING:
		g_ShieldSpeed = 0.5f;       // メイン盾の安定した回転速度
		g_SwordLiftOffset = 0.0f;   // 剣が盾に刺さったままの位置を維持
		break;

	case STATE_PULLING_SWORD:
		// 剣を盾から真上に引き抜く（オフセット値を SWORD_LIFT_DISTANCE まで増やす）
		if (g_SwordLiftOffset < SWORD_LIFT_DISTANCE) {
			g_SwordLiftOffset += 15.0f; // 剣を引き抜く速度
			if (g_SwordLiftOffset > SWORD_LIFT_DISTANCE) g_SwordLiftOffset = SWORD_LIFT_DISTANCE;
		}

		// 盾は急停止せず、慣性摩擦によって徐々に減速する
		if (g_ShieldSpeed > 0.0f) {
			g_ShieldSpeed -= 0.02f;
			if (g_ShieldSpeed < 0.0f) g_ShieldSpeed = 0.0f;
		}

		// 状態遷移条件：剣が完全に引き抜かれ、かつ盾が完全に停止したとき
		if (g_SwordLiftOffset >= SWORD_LIFT_DISTANCE && g_ShieldSpeed <= 0.0f) {
			g_CurrentState = STATE_CHANGING_SHIELD;
		}
		break;

	case STATE_CHANGING_SHIELD:
		// 正式なステージのアルバムインデックスを更新
		g_SelectedStage = g_NextStage;

		// メモリリークを防ぐため、古いディスクのテクスチャオブジェクトを削除
		if (g_pMainShield != nullptr) {
			SAFE_DELETE(g_pMainShield);
		}

		// 選択されたステージの新しいディスクをメインのターンテーブルにロード
		{
			std::wstring mainThumbPath = L"";
			if (g_MaxStages > 0) {
				mainThumbPath = GetThumbnailPath(g_ScoreSummaries[g_SelectedStage].thumbnail);
			}
			else {
				mainThumbPath = L"asset\\texture\\notfound_thumbnail.png";
			}
			g_pMainShield = new Sprite2D(
				{ (SCREEN_WIDTH / 2.0f) - 30.0f , (SCREEN_HEIGHT / 2.0f) + 10.0f },
				{ 250.0f, 250.0f },
				g_ShieldRotation, // テクスチャの回転がカクつかないように現在の回転角度を維持
				{ 1.0f, 1.0f, 1.0f, 1.0f },
				BLENDSTATE_ALFA,
				mainThumbPath.c_str()
			);
		}

		// 盾の切り替え完了直後に、新しい盾に剣を突き刺すステートに移行
		g_CurrentState = STATE_PLUNGING_SWORD;
		break;

	case STATE_PLUNGING_SWORD:
		// 剣を新しい盾に突き刺す（オフセット値を 0 に向かって減らす）
		if (g_SwordLiftOffset > 0.0f) {
			g_SwordLiftOffset -= 15.0f;
			if (g_SwordLiftOffset < 0.0f) g_SwordLiftOffset = 0.0f;
		}

		// 剣が完全に盾に刺さった瞬間（オフセットが0以下）、安定状態（PLAYING）のループに戻る
		if (g_SwordLiftOffset <= 0.0f) {
			g_CurrentState = STATE_PLAYING;
			// ✅ Update/Refresh を呼び出す前に、g_SelectedStage に合わせて g_SelectedScoreIndex を同期する
			// 切り替えたステージと一致する vinylIndex を持つ最初のスコアエントリを検索
			for (int i = 0; i < static_cast<int>(g_ScoreSummaries.size()); i++) {
				if (g_ScoreSummaries[i].vinylIndex == g_SelectedStage) {
					g_SelectedScoreIndex = i;
					break;
				}
			}

			RefreshSelectedScoreText(); // UIテキストを対応する曲に更新
			UpdateBgmFromSelection();   // 正しいステージの音楽を再生
		}
		break;
	}

	// --- パート 3: DirectXグラフィック変数へのパラメータ適用 ---
	// 現在のフレームにおける g_ShieldSpeed に基づき、メイン盾の継続的な回転角度を計算
	if (g_ShieldSpeed > 0.0f) {
		g_ShieldRotation += g_ShieldSpeed;
		if (g_ShieldRotation >= 360.0f) g_ShieldRotation -= 360.0f;
	}

	// Cập nhật xoay cho khiên chính
	if (g_pMainShield != nullptr) {
		g_pMainShield->SetRot(g_ShieldRotation);
	}
	if (g_pShieldFrame != nullptr) {
		g_pShieldFrame->SetRot(g_ShieldRotation);
	}

	// 剣の垂直スクロール移動を更新（オフセットが大きいほど、剣がより高く引き上げられる）
	// 注意：ご使用の環境でY座標が下向きではなく上向きに増加する場合は、符号をマイナスからプラスに変更してください。
	if (g_pSword != nullptr) {
		float diagonalOffset = g_SwordLiftOffset * SWORD_DIAGONAL_FACTOR;
		g_pSword->SetPos({ g_SwordBaseX + diagonalOffset, g_SwordBaseY - diagonalOffset });
	}

	// --- パート 4: 左側の小さなディスク列の処理 ＆ マウスクリック ---
	for (int i = 0; i < g_MaxStages; i++)
	{
		// 位置を計算するために、g_SelectedStageの代わりにg_ScrollOffsetを使用する
		float offset = (float)i - g_ScrollOffset;

		// float形式での円状のラッピング処理
		while (offset < 0.0f)          offset += (float)g_MaxStages;
		while (offset >= (float)g_MaxStages) offset -= (float)g_MaxStages;

		float posX = (SCREEN_WIDTH / 2.0f) - 550.0f;
		float posY = anchorY + (offset * spacing);

		g_pStageShields[i]->SetPos({ posX, posY });

		// 選択中のディスク ＝ オフセットが0に最も近いディスク
		if (i == g_SelectedStage) {
			g_pStageShields[i]->SetRotation(g_ShieldRotation * 2.0f);
		}
		else {
			g_pStageShields[i]->SetRotation(0.0f);
		}

		if (g_pStageShields[i]->IsClick() && g_CurrentState == STATE_PLAYING && g_SelectedStage != i)
		{
			g_NextStage = i;
			g_CurrentState = STATE_PULLING_SWORD;
			if (g_pCurrentBgmData != nullptr) {
				StopSound(g_pCurrentBgmData);
			}
		}
	}

	// --- パート 5: ゲーム開始の決定 (ENTER / SPACE) ---
	if (g_pStartGameText != nullptr) {
		g_pStartGameText->Update();
	}

	// ディスクが安定して再生している場合のみゲーム開始への遷移を許可し、SetPlayJson -> SetSceneFade の順序を厳密に遵守する
	if (g_CurrentState == STATE_PLAYING) {
		if ((g_pStartGameText != nullptr && g_pStartGameText->IsClick()) || Input_IsActionTrigger(INPUT_ACTION_DECIDE)) {
			// 本番のステージに遷移する前に、待機中のBGMを解放する
			if (g_pCurrentBgmData != nullptr) {
				StopSound(g_pCurrentBgmData);
				// UnloadSound(g_pCurrentBgmData);
				g_pCurrentBgmData = nullptr;
			}
			g_LoadedBgmPath = "";

			SetPlayJson(GetSelectedJsonName());
			SetSceneFade(SCENE_GAME);
		}
	}
}

// ==========================================
// 描画関数 (DRAW)
// ==========================================
void StageSelect_Draw(void)
{
	if (g_pResultBG != nullptr) g_pResultBG->Draw();
	g_pBackground->Draw(); // 1. 最背面にレコードプレーヤーの背景を描画
	g_pSword->Draw();      // 4. メイン盾の上に覆いかぶさるように、剣の抜き差し（移動）を描画

	g_pMainShield->Draw(); // 3. 画面中央のメイン回転盾を描画
	if (g_pShieldFrame != nullptr) g_pShieldFrame->Draw();
	// 2. 左側のすべての小さなディスクを描画
	for (int i = 0; i < g_MaxStages; i++)
	{
		if (g_pStageShields[i] != nullptr)
		{
			g_pStageShields[i]->Draw();
			// 続いて、該当する画像の座標と回転角度に正確に追従するように、重ねて外枠を描画する
			if (g_pSmallShieldFrame != nullptr)
			{
				g_pSmallShieldFrame->SetPos(g_pStageShields[i]->GetPos()); // 同一のX, Y座標を適用
				g_pSmallShieldFrame->SetRot(g_pStageShields[i]->GetRot()); // 同一の回転角度を適用（回転する場合）
				g_pSmallShieldFrame->Draw(); // 最前面に重ねて描画
			}
		}
	}

	g_pScoreInfoText->Draw(); // 5. 最前面の右上に曲情報とスコアのJSONテキストを描画

	if (g_pStartGameBG != nullptr) g_pStartGameBG->Draw();
	if (g_pStartGameText != nullptr) g_pStartGameText->Draw();
}

// ==========================================
// メモリ解放関数 (FINALIZE)
// ==========================================
void StageSelect_Finalize(void)
{
	// メモリリークを防ぐため、曲を停止してサウンドデータを解放する
	if (g_pCurrentBgmData != nullptr) {
		StopSound(g_pCurrentBgmData);
		UnloadSound(g_pCurrentBgmData);
		g_pCurrentBgmData = nullptr;
	}
	g_LoadedBgmPath = "";

	SAFE_DELETE(g_pResultBG);
	SAFE_DELETE(g_pBackground);
	SAFE_DELETE(g_pMainShield);
	SAFE_DELETE(g_pShieldFrame);
	SAFE_DELETE(g_pSword);
	SAFE_DELETE(g_pScoreInfoText);
	SAFE_DELETE(g_pSmallShieldFrame); // <--- 新規追加

	for (int i = 0; i < g_MaxStages; i++) {
		SAFE_DELETE(g_pStageShields[i]);
	}
	g_pStageShields.clear();

	g_ScoreSummaries.clear();

	SAFE_DELETE(g_pStartGameBG);
	SAFE_DELETE(g_pStartGameText);
}