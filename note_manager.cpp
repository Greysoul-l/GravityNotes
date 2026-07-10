#include "define.h"
#include "game.h"
#include "note_manager.h"
#include "sound.h"
#include "enemy_note.h"
#include "orb_note.h"
#include "barrier_note.h"
#include "hold_note.h"
#include "rainbow_note.h"

static const float HIT_ZONE_Z     = 3.0f;
static const float PASSIVE_ZONE_Z = 0.5f; // Orb・Barrier の自動判定Z
static const float PERFECT_WINDOW = 1.0f;
static const float GOOD_WINDOW    = 2.5f;
static const float ROPE_ACTIVATE_WINDOW = 0.5f; // レインボーはプレイヤーの足元でのみ活性化（PASSIVE_ZONE_Z基準）

// beat を「そのノーツをスポーンすべき時刻（秒）」に変換
float NoteManager::BeatToSpawnTime(float beat) const
{
	float hitTime    = beat * 60.0f / m_ScoreData.bpm;
	float travelTime = (m_SpawnZ - HIT_ZONE_Z) / m_NoteSpeed;
	return hitTime - travelTime;
}

int NoteManager::WallToFace(ScoreWall wall) const
{
	switch (wall)
	{
	case ScoreWall::Down:  return 0; // FACE_FLOOR
	case ScoreWall::Left:  return 1; // FACE_LEFT_WALL
	case ScoreWall::Up:    return 2; // FACE_CEILING
	case ScoreWall::Right: return 3; // FACE_RIGHT_WALL
	default:               return 0;
	}
}

void NoteManager::Init(const std::string& scoreFilePath)
{
	m_NoteSpeed      = 10.0f;
	m_SpawnZ         = 80.0f;
	m_ElapsedTime    = -3.0f;
	m_NextEventIndex = 0;
	m_BgmStarted     = false;

	m_ScoreData = LoadScore(scoreFilePath);

	std::string bgmPath = "asset\\sound\\bgm\\" + m_ScoreData.music;
	m_pBgmData = LoadMP3(bgmPath);
}

void NoteManager::Update(int playerLane, int playerFace)
{
	if (!m_BgmStarted)
	{
		m_ElapsedTime += dt;
		if (m_ElapsedTime >= 0.0f)
		{
			if (m_pBgmData != nullptr)
			{
				PlaySound(m_pBgmData, false);
			}
			m_BgmStarted = true;
		}
	}
	else
	{
		if (m_pBgmData != nullptr)
		{
			m_ElapsedTime = (float)GetPlaybackPositionSec(m_pBgmData);
		}
		else
		{
			m_ElapsedTime += dt;
		}
	}

	// スポーン処理：時刻が来たイベントを順番に生成
	while (m_NextEventIndex < (int)m_ScoreData.events.size())
	{
		const ScoreEvent& ev = m_ScoreData.events[m_NextEventIndex];
		if (m_ElapsedTime < BeatToSpawnTime(ev.beat)) break;

		int face = WallToFace(ev.wall);

		// 現時刻でノーツが居るべきZ座標を計算（遅延スポーン時は手前に補正）
		float hitTime = ev.beat * 60.0f / m_ScoreData.bpm;
		float initZ   = (hitTime - m_ElapsedTime) * m_NoteSpeed + HIT_ZONE_Z;

		// JSON lane (0=左, 1=中央, 2=右) → ゲーム lane (-1=左, 0=中央, 1=右) に変換
		int gameLane = ev.lane - 1;

		switch (ev.type)
		{
		case ScoreType::Enemy:
		{
			EnemyNote* note = new EnemyNote();
			note->Init(gameLane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Orb:
		{
			OrbNote* note = new OrbNote();
			note->Init(gameLane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Barrier:
		{
			BarrierNote* note = new BarrierNote();
			note->Init(gameLane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Hold:
		{
			float endHitTime = ev.endBeat * 60.0f / m_ScoreData.bpm;
			float endZ       = (endHitTime - m_ElapsedTime) * m_NoteSpeed + HIT_ZONE_Z;

			HoldNote* note = new HoldNote();
			note->Init(ev.lane, ev.endLane, face, initZ, endZ, m_NoteSpeed, m_ScoreData.bpm);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::RopeHold:
		{
			float endHitTime  = ev.endBeat * 60.0f / m_ScoreData.bpm;
			float endZ        = (endHitTime - m_ElapsedTime) * m_NoteSpeed + HIT_ZONE_Z;
			int   endFace     = WallToFace(ev.endWall);
			int   gameEndLane = ev.endLane - 1;

			RopeHoldNote* note = new RopeHoldNote();
			note->Init(gameLane, gameEndLane, face, endFace, initZ, endZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		default:
			break;
		}
		m_NextEventIndex++;
	}

	// 更新・自動判定・削除
	for (int i = (int)m_Notes.size() - 1; i >= 0; i--)
	{
		m_Notes[i]->Update();

		if (!m_Notes[i]->IsHit())
		{
			float z = m_Notes[i]->GetPosZ();

			// Orb: 同 lane・face なら自動取得、違えばMiss
			if (OrbNote* orb = dynamic_cast<OrbNote*>(m_Notes[i]))
			{
				if (z <= PASSIVE_ZONE_Z)
				{
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
						orb->OnHit();
					else
						orb->OnMiss();
				}
			}
			// Barrier: 同 lane・face なら被弾、違えば回避成功
			else if (BarrierNote* barrier = dynamic_cast<BarrierNote*>(m_Notes[i]))
			{
				if (z <= PASSIVE_ZONE_Z)
				{
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
						barrier->OnMiss();
					else
						barrier->OnHit();
				}
			}
			// Enemy: 判定窓を通過したら押し逃しMiss
			// HoldNote・RopeHoldNote は自身で Miss 処理するのでスキップ
			else if (!dynamic_cast<HoldNote*>(m_Notes[i]) &&
			         !dynamic_cast<RopeHoldNote*>(m_Notes[i]) &&
			         z < HIT_ZONE_Z - GOOD_WINDOW)
			{
				m_Notes[i]->OnMiss();
				m_PendingJudges.push(JUDGE_MISS); // StatusManager に伝える
			}
		}

		if (!m_Notes[i]->IsActive())
		{
			// RopeHoldNote 完了時のスコアをキューに積む
			if (RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(m_Notes[i]))
			{
				if (rope->GetState() == RopeHoldNote::State::COMPLETE)
					m_PendingJudges.push(JUDGE_PERFECT);
			}
			delete m_Notes[i];
			m_Notes.erase(m_Notes.begin() + i);
		}
	}
}

void NoteManager::Draw()
{
	for (NoteBase* note : m_Notes)
		note->Draw();
}

void NoteManager::Finalize()
{
	if (m_pBgmData != nullptr) {
		StopSound(m_pBgmData);
		UnloadSound(m_pBgmData);
		m_pBgmData = nullptr;
	}

	for (NoteBase* note : m_Notes)
		delete note;
	m_Notes.clear();
}

JUDGE NoteManager::Judge(int lane, int face)
{
	// Enemy の判定（KeyTrigger）
	NoteBase* bestNote = nullptr;
	float bestDist = FLT_MAX;

	for (NoteBase* note : m_Notes)
	{
		if (!note->IsActive() || note->IsHit()) continue;
		if (note->GetLaneIndex() != lane || note->GetFace() != face) continue;
		if (dynamic_cast<OrbNote*>(note) || dynamic_cast<BarrierNote*>(note)) continue;
		if (dynamic_cast<RopeHoldNote*>(note)) continue; // ロープホールドは別扱い
		if (dynamic_cast<HoldNote*>(note)) continue;     // Hold本体は非対象（子ノートで判定）

		float dist = fabsf(note->GetPosZ() - HIT_ZONE_Z);
		if (dist < bestDist)
		{
			bestDist = dist;
			bestNote = note;
		}
	}

	if (bestNote)
	{
		if (bestDist < PERFECT_WINDOW) { bestNote->OnHit(); return JUDGE_PERFECT; }
		if (bestDist < GOOD_WINDOW)    { bestNote->OnHit(); return JUDGE_GOOD; }
		return JUDGE_NONE;
	}

	// HoldNote（連撃）の最初の一撃はKeyTriggerで取る
	for (NoteBase* note : m_Notes)
	{
		HoldNote* hold = dynamic_cast<HoldNote*>(note);
		if (!hold || !hold->IsActive()) continue;

		EnemyNote* child = hold->GetNearestActiveChild(lane, face);
		if (!child) continue;

		float dist = fabsf(child->GetPosZ() - HIT_ZONE_Z);
		if (dist < PERFECT_WINDOW) { child->OnHit(); return JUDGE_PERFECT; }
		if (dist < GOOD_WINDOW)    { child->OnHit(); return JUDGE_GOOD; }
	}

	return JUDGE_NONE;
}

JUDGE NoteManager::JudgeHold(int lane, int face)
{
	// RopeHoldNote: 足元（PASSIVE_ZONE_Z）に来た時だけKeyDownで活性化（スコアは完了時に加算）
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (!rope || rope->GetState() != RopeHoldNote::State::IDLE) continue;
		if (rope->GetLaneIndex() != lane || rope->GetFace() != face) continue;

		float dist = fabsf(rope->GetPosZ() - PASSIVE_ZONE_Z);
		if (dist < ROPE_ACTIVATE_WINDOW)
		{
			rope->Activate();
			return JUDGE_NONE; // 活性化のみ。スコアは Complete 時に PendingJudge で加算
		}
	}

	// RopeHoldNote が HOLDING 中はスコア加算なし（完了時に PendingJudge で加算）
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (rope && rope->GetState() == RopeHoldNote::State::HOLDING)
			return JUDGE_NONE;
	}

	// HoldNote（連撃）の継続判定（KeyDown）
	for (NoteBase* note : m_Notes)
	{
		HoldNote* hold = dynamic_cast<HoldNote*>(note);
		if (!hold || !hold->IsActive()) continue;

		EnemyNote* child = hold->GetNearestActiveChild(lane, face);
		if (!child) continue;

		float dist = fabsf(child->GetPosZ() - HIT_ZONE_Z);
		if (dist < PERFECT_WINDOW) { child->OnHit(); return JUDGE_PERFECT; }
		if (dist < GOOD_WINDOW)    { child->OnHit(); return JUDGE_GOOD; }
	}
	return JUDGE_NONE; // HoldNote が存在しない／範囲外のときは何もしない
}

RopeHoldNote* NoteManager::GetHoldingRope()
{
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (rope && rope->GetState() == RopeHoldNote::State::HOLDING)
			return rope;
	}
	return nullptr;
}

JUDGE NoteManager::OnButtonRelease(int lane, int face)
{
	for (NoteBase* note : m_Notes)
	{
		RopeHoldNote* rope = dynamic_cast<RopeHoldNote*>(note);
		if (!rope || rope->GetState() != RopeHoldNote::State::HOLDING) continue;

		float progress = rope->GetHoldProgress();
		rope->Release();
		return (progress >= 0.5f) ? JUDGE_GOOD : JUDGE_MISS;
	}
	return JUDGE_NONE;
}