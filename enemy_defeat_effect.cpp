#include "enemy_defeat_effect.h"

#include <cmath>

using namespace DirectX;

namespace
{
	constexpr int FAST_PARTICLE_COUNT = 20;
	constexpr int TRAIL_PARTICLE_COUNT = 12;
	constexpr float PI = 3.14159265f;

	XMFLOAT3 MoveInsideTunnel(XMFLOAT3 position, int face, float distance)
	{
		// 壁面とのZ-fightingや埋まりを避けるため、発生位置を内側へ寄せる。
		switch (face)
		{
		case 0: position.y += distance; break;
		case 1: position.x += distance; break;
		case 2: position.y -= distance; break;
		case 3: position.x -= distance; break;
		}
		return position;
	}

	void AimInsideTunnel(XMFLOAT3& velocity, int face)
	{
		// 壁の外へ飛ぶ速度成分を反転し、エフェクトをトンネル内に見せる。
		switch (face)
		{
		case 0: velocity.y = std::fabs(velocity.y); break;
		case 1: velocity.x = std::fabs(velocity.x); break;
		case 2: velocity.y = -std::fabs(velocity.y); break;
		case 3: velocity.x = -std::fabs(velocity.x); break;
		}
	}

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

EnemyDefeatEffect::EnemyDefeatEffect()
	: ParticleManager("asset/texture/enemy_defeat_particle.png", 256),
	  m_RandomState(0x4A3B2C1Du)
{
}

float EnemyDefeatEffect::Random01()
{
	m_RandomState = m_RandomState * 1664525u + 1013904223u;
	return static_cast<float>((m_RandomState >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

float EnemyDefeatEffect::RandomRange(float minValue, float maxValue)
{
	return minValue + (maxValue - minValue) * Random01();
}

void EnemyDefeatEffect::Spawn(const XMFLOAT3& position, int face)
{
	const XMFLOAT3 particleOrigin = MoveInsideTunnel(position, face, 0.15f);
	const XMFLOAT3 flashOrigin = MoveInsideTunnel(position, face, 0.7f);

	// 中央フラッシュは各面からトンネル内側へ寄せ、どの重力面でも位置を揃える。
	for (int i = 0; i < 3; ++i)
	{
		Particle flash;
		flash.position = flashOrigin;
		flash.startSize = 2.6f - i * 0.55f;
		flash.endSize = 0.15f;
		flash.rotation = RandomRange(0.0f, 360.0f);
		flash.angularVelocity = (i % 2 == 0) ? 260.0f : -320.0f;
		flash.lifetime = 0.20f + i * 0.08f;
		Emit(flash);
	}

	// 高速の大きな破片で、撃破直後の勢いを作る。
	for (int i = 0; i < FAST_PARTICLE_COUNT; ++i)
	{
		const float angle = (2.0f * PI * i / FAST_PARTICLE_COUNT) + RandomRange(-0.16f, 0.16f);
		const float speed = RandomRange(5.0f, 8.5f);

		Particle particle;
		particle.position = particleOrigin;
		particle.velocity = {
			std::cos(angle) * speed,
			std::sin(angle) * speed,
			RandomRange(-2.2f, 2.2f)
		};
		AimInsideTunnel(particle.velocity, face);
		particle.acceleration = GetInsideAcceleration(face, 1.2f);
		particle.startSize = RandomRange(0.35f, 0.72f);
		particle.endSize = 0.03f;
		particle.rotation = RandomRange(0.0f, 360.0f);
		particle.angularVelocity = RandomRange(-620.0f, 620.0f);
		particle.damping = 2.2f;
		particle.lifetime = RandomRange(0.28f, 0.52f);
		Emit(particle);
	}

	// 遅い小粒を長めに残し、爆発後の余韻を作る。
	for (int i = 0; i < TRAIL_PARTICLE_COUNT; ++i)
	{
		const float angle = RandomRange(0.0f, 2.0f * PI);
		const float speed = RandomRange(1.8f, 3.8f);

		Particle particle;
		particle.position = {
			particleOrigin.x + RandomRange(-0.15f, 0.15f),
			particleOrigin.y + RandomRange(-0.15f, 0.15f),
			particleOrigin.z + RandomRange(-0.12f, 0.12f)
		};
		particle.velocity = {
			std::cos(angle) * speed,
			std::sin(angle) * speed,
			RandomRange(-0.8f, 0.8f)
		};
		AimInsideTunnel(particle.velocity, face);
		particle.acceleration = GetInsideAcceleration(face, 0.5f);
		particle.startSize = RandomRange(0.18f, 0.38f);
		particle.endSize = 0.02f;
		particle.rotation = RandomRange(0.0f, 360.0f);
		particle.angularVelocity = RandomRange(-260.0f, 260.0f);
		particle.damping = 1.0f;
		particle.lifetime = RandomRange(0.55f, 0.9f);
		Emit(particle);
	}
}
