#pragma once

#include <cstdint>
#include "particle_manager.h"

// Orb取得時に、Orbと同系色の炎粒子をプレイヤー側へ放出する。
class OrbCollectEffect final : public ParticleManager
{
public:
	OrbCollectEffect();
	void Spawn(const DirectX::XMFLOAT3& position, int face);

private:
	float Random01();
	float RandomRange(float minValue, float maxValue);
	std::uint32_t m_RandomState;
};
