#pragma once
#include "note_manager.h"
#include "scene.h"

class StatusManager
{
private:
	int   m_HP;
	int   m_MaxHP;
	int   m_Score;
	int   m_Combo;
	int   m_MaxCombo;
	int   m_Hits;
	int   m_Misses;
	int   m_OrbGets;
	int   m_OrbLosses;
	JUDGE m_LastJudge   = JUDGE_NONE;
	bool  m_HasNewJudge = false;

public:
	void Init(int maxHP = 1000);
	void Finalize();

	void OnJudge(JUDGE result);
	void OnJudgeHold(JUDGE result);
	void OnOrbHit();
	void OnOrbMiss();

	int  GetHP()       const { return m_HP; }
	int  GetMaxHP()    const { return m_MaxHP; }
	int  GetScore()    const { return m_Score; }
	int  GetCombo()    const { return m_Combo; }
	int  GetMaxCombo() const { return m_MaxCombo; }
	bool IsDead()      const { return m_HP <= 0; }

	bool  HasNewJudge()  const { return m_HasNewJudge; }
	JUDGE ConsumeJudge()       { m_HasNewJudge = false; return m_LastJudge; }

	SendResult GetResult() const;
};
