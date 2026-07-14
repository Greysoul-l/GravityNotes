#pragma once

#include <cstdint>
#include "particle_manager.h"

// エネミー撃破時に中央フラッシュと放射状の破片を生成する。
class EnemyDefeatEffect final : public ParticleManager
{
public:
	EnemyDefeatEffect();
	void Spawn(const DirectX::XMFLOAT3& position, int face);

private:
	float Random01();
	float RandomRange(float minValue, float maxValue);
	std::uint32_t m_RandomState;
};
