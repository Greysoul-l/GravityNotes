#include "status_manager.h"
#include "debug_params.h"

void StatusManager::Init(int maxHP)
{
	m_MaxHP    = maxHP;
	m_HP       = maxHP;
	m_Score    = 0;
	m_Combo    = 0;
	m_MaxCombo = 0;
	m_HitCount  = 0;
	m_MissCount = 0;
}

void StatusManager::Finalize()
{
}

void StatusManager::OnJudge(JUDGE result)
{
	m_LastJudge   = result;
	m_HasNewJudge = true;
	if (result == JUDGE_MISS)
	{
		m_HP-=30;
		m_Combo = 0;
		m_MissCount++;
		return;
	}

	// HIT（PERFECT / GOOD 共通）
	float multiplier = 1.0f + m_Combo * D_PARAMS.comboMultiplier;
	m_Score += static_cast<int>(D_PARAMS.baseScore * multiplier);
	m_Combo++;
	if (m_Combo > m_MaxCombo)
		m_MaxCombo = m_Combo;
	m_HitCount++;
}

void StatusManager::OnJudgeHold(JUDGE result)
{
	OnJudge(result);
}

SendResult StatusManager::GetResult() const
{
	SendResult r;
	r.maxCombo = m_MaxCombo;
	r.hits  = m_HitCount;
	r.misses = m_MissCount;
	return r;
}
