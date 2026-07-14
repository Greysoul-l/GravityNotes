#include "status_manager.h"
#include "debug_params.h"
#include "debug_ostream.h"

void StatusManager::Init(int maxHP)
{
	m_MaxHP    = maxHP;
	m_HP       = maxHP;
	m_Score    = 0;
	m_Combo    = 0;
	m_MaxCombo = 0;
	m_Hits      = 0;
	m_Misses    = 0;
	m_OrbGets   = 0;
	m_OrbLosses = 0;
	m_LastJudge   = JUDGE_NONE;
	m_HasNewJudge = false;
}

void StatusManager::Finalize()
{
}

void StatusManager::OnJudge(JUDGE result)
{
	if (result == JUDGE_NONE)
		return;

	if (result == JUDGE_SILENT_COMBO)
	{
		float multiplier = 1.0f + m_Combo * D_PARAMS.comboMultiplier;
		m_Score += static_cast<int>(D_PARAMS.baseScore * multiplier);
		m_Combo++;
		if (m_Combo > m_MaxCombo)
			m_MaxCombo = m_Combo;
		m_Hits++;
		return;
	}

	m_LastJudge   = result;
	m_HasNewJudge = true;
	if (result == JUDGE_MISS || result == JUDGE_PASS_MISS)
	{
		if (result == JUDGE_MISS)
		{
			m_HP-=30;
		}
		m_Combo = 0;
		m_Misses++;
		return;
	}

	// HIT
	float multiplier = 1.0f + m_Combo * D_PARAMS.comboMultiplier;
	m_Score += static_cast<int>(D_PARAMS.baseScore * multiplier);
	m_Combo++;
	if (m_Combo > m_MaxCombo)
		m_MaxCombo = m_Combo;
	m_Hits++;
}

void StatusManager::OnJudgeHold(JUDGE result)
{
	OnJudge(result);
}

void StatusManager::OnOrbHit()
{
	// スコア・コンボは変化させずHP回復のみ行う
	m_HP += D_PARAMS.orbHealAmount;
	if (m_HP > m_MaxHP)
		m_HP = m_MaxHP;
	m_OrbGets++;

#ifdef _DEBUG
	hal::dout << "[Orb] HIT  HP=" << m_HP << "/" << m_MaxHP << " orbGets=" << m_OrbGets << std::endl;
#endif
}

void StatusManager::OnOrbMiss()
{
	// HP・コンボは変化させず取り逃し数のみ加算する
	m_OrbLosses++;

#ifdef _DEBUG
	hal::dout << "[Orb] MISS orbLosses=" << m_OrbLosses << std::endl;
#endif
}

SendResult StatusManager::GetResult() const
{
	SendResult r;
	r.maxCombo   = m_MaxCombo;
	r.hits       = m_Hits;
	r.misses     = m_Misses;
	r.orbgets    = m_OrbGets;
	r.orblosses  = m_OrbLosses;
	return r;
}
