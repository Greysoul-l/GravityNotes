#include "define.h"
#include "debug_ostream.h"
#include "game.h"
#include "note_manager.h"
#include "options_manager.h"
#include "sound.h"
#include "enemy_note.h"
#include "orb_note.h"
#include "barrier_note.h"
#include "hold_note.h"
#include "rainbow_note.h"
#include "debug_params.h"
#include "enemy_defeat_effect.h"
#include "orb_collect_effect.h"
#include "camera.h"
#include <algorithm>

static const float HIT_ZONE_Z     = 3.0f;
static const float PASSIVE_ZONE_Z = 0.5f; // Orb・Barrier の自動判定Z
static const float HIT_WINDOW     = 2.5f;
static const float ROPE_ACTIVATE_WINDOW = 0.5f; // レインボーはプレイヤーの足元でのみ活性化（PASSIVE_ZONE_Z基準）

// startFace→endFaceの経由面リストを構築する。
// face番号は 床(0)→左壁(1)→天井(2)→右壁(3)→床(0) の順で隣接しており、+1方向がCW、-1方向がCCW。
// hasDirection=false の場合は従来互換の最短経路（2面差＝正面はCW側を採用）
static std::vector<int> BuildRainbowFacePath(int startFace, int endFace, bool hasDirection, RotationDir direction)
{
	std::vector<int> path;
	path.push_back(startFace);
	if (startFace == endFace) return path;

	int step;
	if (hasDirection)
	{
		step = (direction == RotationDir::CW) ? 1 : -1;
	}
	else
	{
		int diffCW = ((endFace - startFace) % 4 + 4) % 4;
		step = (diffCW <= 2) ? 1 : -1; // 対面（2面差）はCW側を既定とする
	}

	int face = startFace;
	while (face != endFace)
	{
		face = ((face + step) % 4 + 4) % 4;
		path.push_back(face);
	}
	return path;
}

// beat を「そのノーツをスポーンすべき時刻（秒）」に変換
float NoteManager::BeatToAudioTime(float beat) const
{
	return beat * 60.0f / m_ScoreData.bpm + m_ScoreData.offset;
}

float NoteManager::BeatToSpawnTime(float beat) const
{
	float hitTime    = BeatToAudioTime(beat);
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

JUDGE NoteManager::JudgeByDistance(NoteBase* note, float targetZ)
{
	float dist = fabsf(note->GetPosZ() - targetZ);
	if (dist >= HIT_WINDOW) return JUDGE_NONE;

	// エネミーが消える前の座標を使って撃破パーティクルを生成する。
	if (m_pEnemyDefeatEffect && note->GetType() == NoteType::Enemy)
		m_pEnemyDefeatEffect->Spawn(note->GetPos(), note->GetFace());

	note->OnHit();
	return JUDGE_HIT;
}

void NoteManager::Init(const std::string& scoreFilePath)
{
	// 撃破エフェクトはNoteManagerが生成から解放まで所有する。
	if (!m_pEnemyDefeatEffect)
		m_pEnemyDefeatEffect = new EnemyDefeatEffect();
	if (!m_pOrbCollectEffect)
		m_pOrbCollectEffect = new OrbCollectEffect();

	m_NoteSpeed      = 10.0f;
	m_SpawnZ         = 80.0f;
	m_BgmStarted     = false;
	m_BgmStartTime   = 0.0f;
	m_IsFadingOut    = false;
	m_FadeOutDuration = 0.0f;
	m_FadeOutTimer   = 0.0f;
	m_FadeOutStartVolume = 1.0f;
	m_HoldingRope    = nullptr;

	m_ScoreData = LoadScore(scoreFilePath);

	// 開始小節位置から再生開始時間を計算（1小節スタート）
	int startMeasure = Options_GetStartMeasure();
	if (startMeasure > 1)
	{
		float startBeat = (startMeasure - 1) * 4.0f;
		m_BgmStartTime = BeatToAudioTime(startBeat);
		m_ElapsedTime  = m_BgmStartTime - 3.0f;
	}
	else
	{
		m_BgmStartTime = 0.0f;
		m_ElapsedTime  = -3.0f;
	}

	// 過去の（プレイ開始時点ですでに通り過ぎているべき）イベントをスキップ
	m_NextEventIndex = 0;
	while (m_NextEventIndex < (int)m_ScoreData.events.size())
	{
		const ScoreEvent& ev = m_ScoreData.events[m_NextEventIndex];
		float hitTime = BeatToAudioTime(ev.beat);
		if (hitTime >= m_BgmStartTime)
		{
			break;
		}
		m_NextEventIndex++;
	}

	std::string bgmPath = ResolveMusicPath(m_ScoreData.music);
	m_pBgmData = LoadMP3(bgmPath);
	m_pOrbGetsSe = LoadMP3("asset/sound/se/orbgets.wav");
	m_pRainbowSe = LoadMP3("asset/sound/se/Rainbow.wav");
	m_RainbowSePlaying = false;

	// プールの初期化
	for (int i = 0; i < MAX_ROPE_POOL; ++i)
	{
		m_RopePool[i] = new RopeHoldNote();
		m_RopePoolInUse[i] = false;
	}
}

RopeHoldNote* NoteManager::AcquireRope()
{
	for (int i = 0; i < MAX_ROPE_POOL; ++i)
	{
		if (!m_RopePoolInUse[i])
		{
			m_RopePoolInUse[i] = true;
			return m_RopePool[i];
		}
	}
	// 万が一プールが足りない場合は警告を出して新規生成(保険)
	hal::dout << "[Warning] Rope pool exhausted! Dynamically allocating one." << std::endl;
	return new RopeHoldNote();
}

void NoteManager::ReleaseRope(RopeHoldNote* rope)
{
	if (!rope) return;
	for (int i = 0; i < MAX_ROPE_POOL; ++i)
	{
		if (m_RopePool[i] == rope)
		{
			m_RopePoolInUse[i] = false;
			return;
		}
	}
	// プール外から動的確保されたものの場合は delete
	delete rope;
}

void NoteManager::Update(int playerLane, int playerFace)
{
	// BGMフェードアウト処理
	if (m_IsFadingOut && m_pBgmData && m_pBgmData->pSourceVoice)
	{
		m_FadeOutTimer += dt;
		float volume = m_FadeOutStartVolume * (1.0f - (m_FadeOutTimer / m_FadeOutDuration));
		if (volume < 0.0f) volume = 0.0f;
		m_pBgmData->pSourceVoice->SetVolume(volume);
	}

	static bool justStartedBGM = false;

	if (!m_BgmStarted)
	{
		m_ElapsedTime += dt;
		if (m_ElapsedTime >= m_BgmStartTime)
		{
			if (m_pBgmData != nullptr)
			{
				PlaySound(m_pBgmData, false, 1.0f, m_BgmStartTime);
			}
			m_BgmStarted = true;
			justStartedBGM = true;
		}
	}
	else
	{
		if (m_pBgmData != nullptr)
		{
			// XAudio2の再生開始直後(数ミリ秒)はSamplesPlayedが更新されず
			// GetPlaybackPositionSecが0を返すレースコンディションがあるため、
			// 再生開始した直後の1フレームだけはdtで時間を進める
			if (justStartedBGM)
			{
				m_ElapsedTime += dt;
				justStartedBGM = false;
			}
			else
			{
				static int syncCounter = 0;
				syncCounter++;
				if (syncCounter >= 10)
				{
					m_ElapsedTime = (float)GetPlaybackPositionSec(m_pBgmData);
					syncCounter = 0;
				}
				else
				{
					m_ElapsedTime += dt;
				}
			}
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
		float hitTime = BeatToAudioTime(ev.beat);
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
			note->Init(gameLane, face, initZ, m_NoteSpeed, ev.beat);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Hold:
		{
			float endHitTime = BeatToAudioTime(ev.endBeat);
			float endZ       = (endHitTime - m_ElapsedTime) * m_NoteSpeed + HIT_ZONE_Z;

			HoldNote* note = new HoldNote();
			note->Init(ev.lane, ev.endLane, face, initZ, endZ, m_NoteSpeed, m_ScoreData.bpm, ev.beat, ev.endBeat);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::RopeHold:
		{
			float endHitTime  = BeatToAudioTime(ev.endBeat);
			float endZ        = (endHitTime - m_ElapsedTime) * m_NoteSpeed + HIT_ZONE_Z;
			int   endFace     = WallToFace(ev.endWall);
			int   gameEndLane = ev.endLane - 1;

			std::vector<int> facePath = BuildRainbowFacePath(face, endFace, ev.hasDirection, ev.direction);

			RopeHoldNote* note = AcquireRope();
			note->Init(gameLane, gameEndLane, facePath, initZ, endZ, m_NoteSpeed);
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

			// Orb: HIT_ZONE_Zよりorb JudgeWindow分手前（Z値が大きい＝プレイヤーから遠い）からHIT判定を開始する。
			// プレイヤーの足元(z=0付近)まで引き寄せず、胴あたりの高さで消えているように見せるための猶予。
			// Miss確定ラインは HIT_ZONE_Z - HIT_WINDOW のまま変えない（近すぎるところまで表示され続けるのを防ぐ）
			if (m_Notes[i]->GetType() == NoteType::Orb)
			{
				OrbNote* orb = static_cast<OrbNote*>(m_Notes[i]);
				if (z <= HIT_ZONE_Z + D_PARAMS.orbJudgeWindow)
				{
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
					{
						// Orbが消える前の表示座標から取得パーティクルを生成する。
						if (m_pOrbCollectEffect)
							m_pOrbCollectEffect->Spawn(orb->GetEffectPosition(), orb->GetFace());
						orb->OnHit();
						m_PendingOrbEvents.push(ORB_EVENT_HIT);
						if (m_pOrbGetsSe != nullptr)
						{
							PlaySound(m_pOrbGetsSe, false);
						}
					}
					else if (z < HIT_ZONE_Z - HIT_WINDOW)
					{
						orb->OnMiss();
						m_PendingOrbEvents.push(ORB_EVENT_MISS);
					}
				}
			}
			// Barrier: タイミングはEnemyノーツと一緒（z < HIT_ZONE_Z - HIT_WINDOW）
			else if (m_Notes[i]->GetType() == NoteType::Barrier)
			{
				BarrierNote* barrier = static_cast<BarrierNote*>(m_Notes[i]);
				if (z < HIT_ZONE_Z - HIT_WINDOW)
				{
					float beat = barrier->GetBeat();
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
					{
						barrier->OnMiss();
						m_PendingJudges.push(JUDGE_MISS);
						m_ProcessedBarrierBeats.insert(beat);
					}
					else
					{
						// 操作をしなかった（元から安全な場所にいた）場合は、音や判定を出さずに自然消滅
						barrier->OnHit();
						if (m_ProcessedBarrierBeats.find(beat) == m_ProcessedBarrierBeats.end())
						{
							m_PendingJudges.push(JUDGE_SILENT_COMBO);
							m_ProcessedBarrierBeats.insert(beat);
						}
					}
				}
			}
			// Enemy: 判定窓を通過したら押し逃しMiss
			// HoldNote・RopeHoldNote は自身で Miss 処理するのでスキップ
			else if (m_Notes[i]->GetType() != NoteType::Hold &&
			         m_Notes[i]->GetType() != NoteType::RopeHold &&
			         z < HIT_ZONE_Z - HIT_WINDOW)
			{
				m_Notes[i]->OnMiss();
				m_PendingJudges.push(JUDGE_PASS_MISS); // StatusManager に伝える
			}
		}

		if (!m_Notes[i]->IsActive())
		{
			// RopeHoldNote 完了時のスコアをキューに積む
			if (m_Notes[i]->GetType() == NoteType::RopeHold)
			{
				RopeHoldNote* rope = static_cast<RopeHoldNote*>(m_Notes[i]);
				if (rope->GetState() == RopeHoldNote::State::COMPLETE)
					m_PendingJudges.push(JUDGE_HIT);
				if (m_HoldingRope == rope)
				{
					m_HoldingRope = nullptr;
				}
				ReleaseRope(rope);
			}
			else
			{
				delete m_Notes[i];
			}
			m_Notes.erase(m_Notes.begin() + i);
		}
	}

	// RopeHoldNote (Rainbow) の再生・フェードアウト制御
	RopeHoldNote* holdingRope = m_HoldingRope;
	if (holdingRope != nullptr)
	{
		if (!m_RainbowSePlaying)
		{
			if (m_pRainbowSe != nullptr)
			{
				PlaySound(m_pRainbowSe, true);
			}
			m_RainbowSePlaying = true;
		}

		// 音量のフェードアウト制御 (進捗85%以降で音量1.0から0.0へ減衰)
		float progress = holdingRope->GetHoldProgress();
		float vol = 1.0f;
		if (progress >= 0.85f)
		{
			vol = (1.0f - progress) / 0.15f;
			if (vol < 0.0f) vol = 0.0f;
		}

		if (m_pRainbowSe != nullptr && m_pRainbowSe->pSourceVoice != nullptr)
		{
			m_pRainbowSe->pSourceVoice->SetVolume(vol * SOUND_SE_VOLUME);
		}
	}
	else
	{
		if (m_RainbowSePlaying)
		{
			if (m_pRainbowSe != nullptr)
			{
				StopSound(m_pRainbowSe);
				if (m_pRainbowSe->pSourceVoice != nullptr)
				{
					m_pRainbowSe->pSourceVoice->SetVolume(SOUND_SE_VOLUME);
				}
			}
			m_RainbowSePlaying = false;
		}
	}

	// ノーツが消えた後も、残っている粒子は寿命まで更新する。
	if (m_pEnemyDefeatEffect)
		m_pEnemyDefeatEffect->Update(dt);
	if (m_pOrbCollectEffect)
		m_pOrbCollectEffect->Update(dt);
}

void NoteManager::Draw()
{
	std::vector<OrbNote*> sortedOrbs;
	sortedOrbs.reserve(m_Notes.size());

	// 不透明ノーツを先に描き、透過Orbは深度ソート用に分ける。
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() == NoteType::Orb)
			sortedOrbs.push_back(static_cast<OrbNote*>(note));
		else
			note->Draw();
	}

	const XMMATRIX view = GetCamera()->GetView();
	std::stable_sort(
		sortedOrbs.begin(),
		sortedOrbs.end(),
		[&view](const OrbNote* lhs, const OrbNote* rhs)
		{
			return lhs->GetDrawDepth(view) > rhs->GetDrawDepth(view);
		});
	for (OrbNote* orb : sortedOrbs)
		orb->Draw();

	// ノーツより後に描き、撃破エフェクトを手前へ見せる。
	if (m_pEnemyDefeatEffect)
		m_pEnemyDefeatEffect->Draw();
	if (m_pOrbCollectEffect)
		m_pOrbCollectEffect->Draw();
}

void NoteManager::DrawShadowMapForFace(int face, const XMMATRIX& lightView, const XMMATRIX& lightProjection)
{
	// 指定された面のEnemyモデルとOrbビルボードをShadowMapへ描く。
	for (NoteBase* note : m_Notes)
	{
		if (!note->IsActive() || note->IsHit()) continue;
		if (note->GetFace() != face) continue;
		NoteType type = note->GetType();
		if (type == NoteType::Enemy || type == NoteType::Orb)
		{
			note->DrawShadowMap(lightView, lightProjection);
		}
	}
}

void NoteManager::Finalize()
{
	if (m_pBgmData != nullptr) {
		StopSound(m_pBgmData);
		UnloadSound(m_pBgmData);
		m_pBgmData = nullptr;
	}
	if (m_pOrbGetsSe != nullptr) {
		UnloadSound(m_pOrbGetsSe);
		m_pOrbGetsSe = nullptr;
	}
	if (m_pRainbowSe != nullptr) {
		StopSound(m_pRainbowSe);
		UnloadSound(m_pRainbowSe);
		m_pRainbowSe = nullptr;
	}
	m_RainbowSePlaying = false;
	m_HoldingRope = nullptr;

	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() == NoteType::RopeHold)
		{
			ReleaseRope(static_cast<RopeHoldNote*>(note));
		}
		else
		{
			delete note;
		}
	}
	m_Notes.clear();

	// プール内のインスタンス自体の解放
	for (int i = 0; i < MAX_ROPE_POOL; ++i)
	{
		if (m_RopePool[i])
		{
			delete m_RopePool[i];
			m_RopePool[i] = nullptr;
		}
		m_RopePoolInUse[i] = false;
	}

	delete m_pEnemyDefeatEffect;
	m_pEnemyDefeatEffect = nullptr;
	delete m_pOrbCollectEffect;
	m_pOrbCollectEffect = nullptr;
	RopeHoldNote::FinalizeSharedResources();
}

JUDGE NoteManager::Judge(int lane, int face)
{
	// Enemy の判定（KeyTrigger）
	NoteBase* bestNote = nullptr;
	float bestDist = FLT_MAX;

	for (NoteBase* note : m_Notes)
	{
		if (!note->IsActive() || note->IsHit()) continue;
		if (!IsSameOrCornerPosition(note->GetLaneIndex(), note->GetFace(), lane, face)) continue;
		NoteType type = note->GetType();
		if (type == NoteType::Orb || type == NoteType::Barrier) continue;
		if (type == NoteType::RopeHold) continue; // ロープホールドは別扱い
		if (type == NoteType::Hold) continue;     // Hold本体は非対象（子ノートで判定）

		float dist = fabsf(note->GetPosZ() - HIT_ZONE_Z);
		if (dist < bestDist)
		{
			bestDist = dist;
			bestNote = note;
		}
	}

	if (bestNote)
	{
		return JudgeByDistance(bestNote, HIT_ZONE_Z);
	}

	// HoldNote（連撃）の最初の一撃はKeyTriggerで取る
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() != NoteType::Hold || !note->IsActive()) continue;
		HoldNote* hold = static_cast<HoldNote*>(note);

		EnemyNote* child = hold->GetNearestActiveChild(lane, face);
		if (!child) continue;

		JUDGE j = JudgeByDistance(child, HIT_ZONE_Z);
		if (j != JUDGE_NONE) return j;
	}

	return JUDGE_NONE;
}

JUDGE NoteManager::JudgeHold(int lane, int face)
{
	// RopeHoldNote: 足元（PASSIVE_ZONE_Z）に来た時だけKeyDownで活性化（スコアは完了時に加算）
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() != NoteType::RopeHold) continue;
		RopeHoldNote* rope = static_cast<RopeHoldNote*>(note);
		if (rope->GetState() != RopeHoldNote::State::IDLE) continue;
		if (rope->GetLaneIndex() != lane || rope->GetFace() != face) continue;

		float dist = fabsf(rope->GetPosZ() - PASSIVE_ZONE_Z);
		if (dist < ROPE_ACTIVATE_WINDOW)
		{
			rope->Activate();
			m_HoldingRope = rope;
			return JUDGE_NONE; // 活性化のみ。スコアは Complete 時に PendingJudge で加算
		}
	}

	// RopeHoldNote が HOLDING 中はスコア加算なし（完了時に PendingJudge で加算）
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() != NoteType::RopeHold) continue;
		RopeHoldNote* rope = static_cast<RopeHoldNote*>(note);
		if (rope->GetState() == RopeHoldNote::State::HOLDING)
			return JUDGE_NONE;
	}

	// HoldNote（連撃）の継続判定（KeyDown）
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() != NoteType::Hold || !note->IsActive()) continue;
		HoldNote* hold = static_cast<HoldNote*>(note);

		EnemyNote* child = hold->GetNearestActiveChild(lane, face);
		if (!child) continue;

		JUDGE j = JudgeByDistance(child, HIT_ZONE_Z);
		if (j != JUDGE_NONE) return j;
	}
	return JUDGE_NONE; // HoldNote が存在しない／範囲外のときは何もしない
}

JUDGE NoteManager::OnButtonRelease(int lane, int face)
{
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() != NoteType::RopeHold) continue;
		RopeHoldNote* rope = static_cast<RopeHoldNote*>(note);
		if (rope->GetState() != RopeHoldNote::State::HOLDING) continue;

		float progress = rope->GetHoldProgress();
		rope->Release();
		if (m_HoldingRope == rope)
		{
			m_HoldingRope = nullptr;
		}
		return (progress >= 0.5f) ? JUDGE_HIT : JUDGE_MISS;
	}
	return JUDGE_NONE;
}

bool NoteManager::IsFinished() const
{
	return (m_NextEventIndex >= (int)m_ScoreData.events.size()) && m_Notes.empty();
}

void NoteManager::StartBgmFadeOut(float durationSec)
{
	if (durationSec <= 0.0f)
	{
		durationSec = dt;
	}

	m_IsFadingOut = true;
	m_FadeOutDuration = durationSec;
	m_FadeOutTimer = 0.0f;
	m_FadeOutStartVolume = 1.0f;

	if (m_pBgmData && m_pBgmData->pSourceVoice)
	{
		m_pBgmData->pSourceVoice->GetVolume(&m_FadeOutStartVolume);
	}
}

bool NoteManager::CheckAndHitBarrier(int fromLane, int fromFace, int toLane, int toFace)
{
	// 1. 移動元にバリアがあるかチェック（回避成功）
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() != NoteType::Barrier) continue;
		BarrierNote* barrier = static_cast<BarrierNote*>(note);
		if (!barrier->IsActive() || barrier->IsHit()) continue;
		// 回避成功判定のみ角ペアを許容（被弾判定は完全一致のまま）
		if (!IsSameOrCornerPosition(barrier->GetLaneIndex(), barrier->GetFace(), fromLane, fromFace)) continue;

		float dist = fabsf(barrier->GetPosZ() - HIT_ZONE_Z);
		if (dist < HIT_WINDOW)
		{
			barrier->OnHit();
			float beat = barrier->GetBeat();
			if (m_ProcessedBarrierBeats.find(beat) == m_ProcessedBarrierBeats.end())
			{
				m_PendingJudges.push(JUDGE_KAIHI);
				m_ProcessedBarrierBeats.insert(beat);
			}
			m_PendingBarrierEvents.push(BARRIER_EVENT_KAIHI);
			return true;
		}
	}

	// 2. 移動先にバリアがあるかチェック（被弾）
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() != NoteType::Barrier) continue;
		BarrierNote* barrier = static_cast<BarrierNote*>(note);
		if (!barrier->IsActive() || barrier->IsHit()) continue;
		if (barrier->GetLaneIndex() != toLane || barrier->GetFace() != toFace) continue;

		float dist = fabsf(barrier->GetPosZ() - HIT_ZONE_Z);
		if (dist < HIT_WINDOW)
		{
			barrier->OnMiss();
			m_PendingJudges.push(JUDGE_MISS);
			float beat = barrier->GetBeat();
			m_ProcessedBarrierBeats.insert(beat);
			return true;
		}
	}
	return false;
}

void NoteManager::ResetPlayPosition()
{
	m_HoldingRope = nullptr;
	// 1. アクティブなノーツの削除
	for (NoteBase* note : m_Notes)
	{
		if (note->GetType() == NoteType::RopeHold)
		{
			ReleaseRope(static_cast<RopeHoldNote*>(note));
		}
		else
		{
			delete note;
		}
	}
	m_Notes.clear();

	// プール使用状況のリセット
	for (int i = 0; i < MAX_ROPE_POOL; ++i)
	{
		m_RopePoolInUse[i] = false;
	}

	// 2. ペンディングキューや処理済みバリアビートのクリア
	while (!m_PendingJudges.empty()) m_PendingJudges.pop();
	while (!m_PendingOrbEvents.empty()) m_PendingOrbEvents.pop();
	while (!m_PendingBarrierEvents.empty()) m_PendingBarrierEvents.pop();
	m_ProcessedBarrierBeats.clear();

	// 3. 音声の停止
	if (m_pBgmData != nullptr) {
		StopSound(m_pBgmData);
	}
	if (m_pRainbowSe != nullptr) {
		StopSound(m_pRainbowSe);
	}
	m_RainbowSePlaying = false;
	m_IsFadingOut = false;
	m_FadeOutTimer = 0.0f;

	// 4. 開始小節位置から再生開始時間を再計算
	m_BgmStarted = false;
	int startMeasure = Options_GetStartMeasure();
	if (startMeasure > 1)
	{
		float startBeat = (startMeasure - 1) * 4.0f;
		m_BgmStartTime = BeatToAudioTime(startBeat);
		m_ElapsedTime  = m_BgmStartTime - 3.0f;
	}
	else
	{
		m_BgmStartTime = 0.0f;
		m_ElapsedTime  = -3.0f;
	}

	// 5. 次回イベントインデックスのリセット
	m_NextEventIndex = 0;
	while (m_NextEventIndex < (int)m_ScoreData.events.size())
	{
		const ScoreEvent& ev = m_ScoreData.events[m_NextEventIndex];
		float hitTime = BeatToAudioTime(ev.beat);
		if (hitTime >= m_BgmStartTime)
		{
			break;
		}
		m_NextEventIndex++;
	}
}
