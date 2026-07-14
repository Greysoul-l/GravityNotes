#include "orb_collect_effect.h"

#include <cmath>

using namespace DirectX;

namespace
{
	constexpr int FAST_PARTICLE_COUNT = 32;
	constexpr int TRAIL_PARTICLE_COUNT = 18;
	constexpr float PLAYER_SIDE_SPAWN_OFFSET = 0.45f;
	constexpr float PI = 3.14159265f;

	XMFLOAT3 GetInsideAcceleration(int face, float strength)
	{
		switch (face)
		{
		case 0: return { 0.0f,  strength, 0.0f };
		case 1: return { strength, 0.0f,  0.0f };
		case 2: return { 0.0f, -strength, 0.0f };
		case 3: return { -strength, 0.0f, 0.0f };
		default: return { 0.0f, 0.0f, 0.0f };
		}
	}
}

OrbCollectEffect::OrbCollectEffect()
	: ParticleManager("asset/texture/orb_collect_particle.png", 320),
	  m_RandomState(0x13579BDFu)
{
}

float OrbCollectEffect::Random01()
{
	m_RandomState = m_RandomState * 1664525u + 1013904223u;
	return static_cast<float>((m_RandomState >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

float OrbCollectEffect::RandomRange(float minValue, float maxValue)
{
	return minValue + (maxValue - minValue) * Random01();
}

void OrbCollectEffect::Spawn(const XMFLOAT3& position, int face)
{
	// 大きさと回転が異なる発光を重ね、取得した瞬間を強調する。
	for (int i = 0; i < 3; ++i)
	{
		Particle flash;
		flash.position = position;
		// 発光の中心も少しプレイヤー側へ寄せ、取得位置とのつながりを保つ。
		flash.position.z -= PLAYER_SIDE_SPAWN_OFFSET * 0.5f;
		flash.startSize = 2.2f - i * 0.5f;
		flash.endSize = 0.08f;
		flash.rotation = RandomRange(-40.0f, 40.0f);
		flash.angularVelocity = (i % 2 == 0) ? 260.0f : -320.0f;
		flash.lifetime = 0.18f + i * 0.07f;
		Emit(flash);
	}

	// 高速の炎を大きく放射し、取得直後の勢いを作る。
	for (int i = 0; i < FAST_PARTICLE_COUNT; ++i)
	{
		const float angle = (2.0f * PI * i / FAST_PARTICLE_COUNT) + RandomRange(-0.16f, 0.16f);
		const float radialSpeed = RandomRange(2.5f, 5.5f);

		Particle particle;
		// 発生地点をプレイヤー側へ寄せ、Orb消滅後も手元で粒子が見えるようにする。
		particle.position = {
			position.x + RandomRange(-0.12f, 0.12f),
			position.y + RandomRange(-0.12f, 0.12f),
			position.z - PLAYER_SIDE_SPAWN_OFFSET + RandomRange(-0.12f, 0.12f)
		};
		particle.velocity = {
			std::cos(angle) * radialSpeed,
			std::sin(angle) * radialSpeed,
			RandomRange(-6.0f, -3.0f)
		};
		particle.acceleration = GetInsideAcceleration(face, 0.9f);
		particle.startSize = RandomRange(0.25f, 0.55f);
		particle.endSize = 0.02f;
		particle.rotation = RandomRange(-50.0f, 50.0f);
		particle.angularVelocity = RandomRange(-420.0f, 420.0f);
		particle.damping = 1.9f;
		particle.lifetime = RandomRange(0.3f, 0.55f);
		Emit(particle);
	}

	// 遅い小粒を長く残し、プレイヤーへ吸収される余韻を作る。
	for (int i = 0; i < TRAIL_PARTICLE_COUNT; ++i)
	{
		const float angle = RandomRange(0.0f, 2.0f * PI);
		const float radialSpeed = RandomRange(0.8f, 2.0f);

		Particle particle;
		// 余韻は高速粒子より少し手前から出し、プレイヤー付近の密度を高める。
		particle.position = {
			position.x + RandomRange(-0.18f, 0.18f),
			position.y + RandomRange(-0.18f, 0.18f),
			position.z - PLAYER_SIDE_SPAWN_OFFSET * 1.5f + RandomRange(-0.18f, 0.18f)
		};
		particle.velocity = {
			std::cos(angle) * radialSpeed,
			std::sin(angle) * radialSpeed,
			RandomRange(-3.0f, -1.0f)
		};
		particle.acceleration = GetInsideAcceleration(face, 0.35f);
		particle.startSize = RandomRange(0.15f, 0.32f);
		particle.endSize = 0.01f;
		particle.rotation = RandomRange(-45.0f, 45.0f);
		particle.angularVelocity = RandomRange(-180.0f, 180.0f);
		particle.damping = 0.9f;
		particle.lifetime = RandomRange(0.55f, 0.9f);
		Emit(particle);
	}
}
