#pragma once
#include <vector>
#include <queue>
#include "note_base.h"
#include "scoreloader.h"

class RopeHoldNote;

enum JUDGE {
	JUDGE_NONE = -1,
	JUDGE_PERFECT,
	JUDGE_GOOD,
	JUDGE_MISS
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
	bool      m_BgmStarted = false;

	std::queue<JUDGE> m_PendingJudges;

	float BeatToSpawnTime(float beat) const;
	int   WallToFace(ScoreWall wall)  const;

public:
	void  Init(const std::string& scoreFilePath);
	void  Update(int playerLane, int playerFace);
	void  Draw();
	void  Finalize();
	float GetNoteSpeed() const { return m_NoteSpeed; }
	float GetBPM() const { return m_ScoreData.bpm; }
	float GetElapsedTime() const { return m_ElapsedTime; }

	JUDGE Judge(int lane, int face);
	JUDGE JudgeHold(int lane, int face);
	JUDGE OnButtonRelease(int lane, int face);

	RopeHoldNote* GetHoldingRope();

	bool  HasPendingJudge()  const { return !m_PendingJudges.empty(); }
	JUDGE PopPendingJudge()        { JUDGE j = m_PendingJudges.front(); m_PendingJudges.pop(); return j; }
};