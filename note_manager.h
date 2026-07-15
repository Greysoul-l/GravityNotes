#pragma once
#include <vector>
#include <queue>
#include <set>
#include "note_base.h"
#include "scoreloader.h"

class RopeHoldNote;
class EnemyDefeatEffect;
class OrbCollectEffect;

enum JUDGE {
	JUDGE_NONE = -1,
	JUDGE_HIT,
	JUDGE_MISS,
	JUDGE_KAIHI,
	JUDGE_SILENT_COMBO,
	JUDGE_PASS_MISS
};

enum ORB_EVENT {
	ORB_EVENT_HIT,
	ORB_EVENT_MISS
};

enum BARRIER_EVENT {
	BARRIER_EVENT_NONE = 0,
	BARRIER_EVENT_KAIHI
};

struct SoundData;

class NoteManager
{
private:
	std::vector<NoteBase*> m_Notes;
	float    m_NoteSpeed;
	float    m_SpawnZ;

	ScoreData m_ScoreData;
	float     m_ElapsedTime;
	int       m_NextEventIndex;
	SoundData* m_pBgmData = nullptr;
	SoundData* m_pOrbGetsSe = nullptr;
	SoundData* m_pRainbowSe = nullptr;
	bool      m_BgmStarted = false;
	float     m_BgmStartTime = 0.0f;
	bool      m_RainbowSePlaying = false;
	EnemyDefeatEffect* m_pEnemyDefeatEffect = nullptr;
	OrbCollectEffect* m_pOrbCollectEffect = nullptr;
	RopeHoldNote* m_HoldingRope = nullptr;

	std::queue<JUDGE> m_PendingJudges;
	std::queue<ORB_EVENT> m_PendingOrbEvents;
	std::queue<BARRIER_EVENT> m_PendingBarrierEvents;
	std::set<float> m_ProcessedBarrierBeats;

	float m_FadeOutDuration = 0.0f;
	float m_FadeOutTimer = 0.0f;
	float m_FadeOutStartVolume = 1.0f;
	bool  m_IsFadingOut = false;

	static constexpr int MAX_ROPE_POOL = 4;
	RopeHoldNote* m_RopePool[MAX_ROPE_POOL] = {};
	bool          m_RopePoolInUse[MAX_ROPE_POOL] = {};

	RopeHoldNote* AcquireRope();
	void          ReleaseRope(RopeHoldNote* rope);

	float BeatToSpawnTime(float beat) const;
	// beatを、offset補正込みの「曲再生位置における実時刻（秒）」に変換する
	float BeatToAudioTime(float beat) const;
	int   WallToFace(ScoreWall wall)  const;
	JUDGE JudgeByDistance(NoteBase* note, float targetZ);

public:
	void  Init(const std::string& scoreFilePath);
	void  Update(int playerLane, int playerFace);
	void  Draw();
	// 指定した面(0=FLOOR,1=LEFT_WALL,2=CEILING,3=RIGHT_WALL)にいるEnemy/Orbの影を描く。
	void  DrawShadowMapForFace(int face, const XMMATRIX& lightView, const XMMATRIX& lightProjection);
	void  Finalize();
	void  ResetPlayPosition();
	float GetNoteSpeed() const { return m_NoteSpeed; }
	float GetBPM() const { return m_ScoreData.bpm; }
	float GetElapsedTime() const { return m_ElapsedTime; }
	const ScoreData& GetScoreData() const { return m_ScoreData; }

	JUDGE Judge(int lane, int face);
	JUDGE JudgeHold(int lane, int face);
	JUDGE OnButtonRelease(int lane, int face);

	RopeHoldNote* GetHoldingRope() const { return m_HoldingRope; }

	bool  HasPendingJudge()  const { return !m_PendingJudges.empty(); }
	JUDGE PopPendingJudge()        { JUDGE j = m_PendingJudges.front(); m_PendingJudges.pop(); return j; }

	bool      HasPendingOrbEvent() const { return !m_PendingOrbEvents.empty(); }
	ORB_EVENT PopPendingOrbEvent()       { ORB_EVENT e = m_PendingOrbEvents.front(); m_PendingOrbEvents.pop(); return e; }
	bool          HasPendingBarrierEvent() const { return !m_PendingBarrierEvents.empty(); }
	BARRIER_EVENT PopPendingBarrierEvent()       { BARRIER_EVENT e = m_PendingBarrierEvents.front(); m_PendingBarrierEvents.pop(); return e; }
	bool  CheckAndHitBarrier(int fromLane, int fromFace, int toLane, int toFace);
	bool  IsFinished() const;
	void  StartBgmFadeOut(float durationSec);
};
