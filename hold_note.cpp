#include "hold_note.h"
#include "game.h"
#include <cmath>

// ======================================================
// 子ノートの間隔（拍数）― ここを変えると密度が変わる
//   0.25f = 1/4拍ごと（デフォルト）
//   0.125f = 1/8拍ごと（より細かい）
//   0.5f  = 1/2拍ごと（より荒い）
// ======================================================
static const float HOLD_BEAT_INTERVAL = 0.25f;

// NoteManager 側の定数と揃えること
static const float HOLD_HIT_ZONE_Z  = 3.0f;
static const float HOLD_HIT_WINDOW  = 2.5f;

// 子ノートの大きさ（EnemyNote::Init() で設定される基準スケールに対する倍率）
static const float HOLD_FIRST_CHILD_SCALE_MULT = 1.1f;  // 一体目は少し大きく
static const float HOLD_CHILD_SCALE_MULT       = 0.8f; // それ以外は少し小さく

HoldNote::~HoldNote()
{
	for (EnemyNote* child : m_ChildNotes)
		delete child;
	m_ChildNotes.clear();
}

void HoldNote::Init(int lane, int endLane, int face, float initZ, float endZ, float speed, float bpm)
{
	m_Face     = face;
	m_Speed    = speed;
	m_IsActive = true;
	m_IsHit    = false;

	// JSON lane (0=左, 1=中央, 2=右) → ゲーム lane (-1=左, 0=中央, 1=右) に変換
	int gameLane    = lane    - 1;
	int gameEndLane = endLane - 1;

	// 1子ノートあたりのZ間隔 = beat間隔(拍) × (60秒/bpm) × speed
	float zStep = HOLD_BEAT_INTERVAL * (60.0f / bpm) * speed;

	// 始点〜終点をzStepで分割して子ノート数を決定
	int totalSteps = (zStep > 0.0f) ? (int)ceilf((endZ - initZ) / zStep) + 1 : 2;
	if (totalSteps < 2) totalSteps = 2;

	for (int i = 0; i < totalSteps; i++)
	{
		// t: 0.0（始点gameLane）〜 1.0（終点gameEndLane）の進捗
		float t = (float)i / (float)(totalSteps - 1);

		// 理想レーンを線形補間し、最寄りレーン(-1〜1)にスナップ
		float idealLane = gameLane + t * (gameEndLane - gameLane);
		int snapLane = (int)roundf(idealLane);
		snapLane = (snapLane < -1) ? -1 : (snapLane > 1) ? 1 : snapLane;

		float childZ = initZ + (float)i * zStep;

		EnemyNote* child = new EnemyNote();
		// 一体目も共通モデルを使用し、テクスチャのみを差し替える
		child->Init(snapLane, face, childZ, speed, nullptr);
		if (i == 0)
		{
			child->SetCustomTexture("asset/texture/Gargoyle_red.png");
		}
		else
		{
			child->SetCustomTexture("asset/texture/Gargoyle_white.png");
		}

		// 一体目だけ少し大きく、それ以外は少し小さく表示する
		float scaleMult = (i == 0) ? HOLD_FIRST_CHILD_SCALE_MULT : HOLD_CHILD_SCALE_MULT;
		XMFLOAT3 baseScale = child->GetScale();
		child->SetSize({ baseScale.x * scaleMult, baseScale.y * scaleMult, baseScale.z * scaleMult });

		m_ChildNotes.push_back(child);
	}
}

void HoldNote::Update()
{
	bool anyActive = false;

	for (EnemyNote* child : m_ChildNotes)
	{
		if (!child->IsActive()) continue;

		child->Update();

		// 判定窓を通過したら押し逃しMiss
		if (!child->IsHit() && child->GetPosZ() < HOLD_HIT_ZONE_Z - HOLD_HIT_WINDOW)
			child->OnMiss();

		if (child->IsActive())
			anyActive = true;
	}

	// 全子ノートが非アクティブになったら自身も非アクティブ
	if (!anyActive)
		m_IsActive = false;
}

void HoldNote::Draw()
{
	for (EnemyNote* child : m_ChildNotes)
	{
		if (child->IsActive())
			child->Draw();
	}
}

EnemyNote* HoldNote::GetNearestActiveChild(int lane, int face) const
{
	EnemyNote* nearest = nullptr;
	float minDist = FLT_MAX;

	for (EnemyNote* child : m_ChildNotes)
	{
		if (!child->IsActive() || child->IsHit()) continue;
		if (!IsSameOrCornerPosition(child->GetLaneIndex(), child->GetFace(), lane, face)) continue;

		float dist = fabsf(child->GetPosZ() - HOLD_HIT_ZONE_Z);
		if (dist < minDist)
		{
			minDist = dist;
			nearest = child;
		}
	}
	return nearest;
}
