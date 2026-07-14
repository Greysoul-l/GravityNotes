#pragma once

#include <cstddef>
#include <vector>
#include <DirectXMath.h>

class Billboard;

// パーティクル共通処理。派生クラスは Emit() を使って用途別の粒子を生成する。
class ParticleManager
{
public:
	ParticleManager(const char* texturePath, std::size_t maxParticles);
	virtual ~ParticleManager();

	virtual void Update(float deltaTime);
	virtual void Draw();

	bool IsEmpty() const { return m_Particles.empty(); }

protected:
	// 1粒子分の移動・拡縮・寿命パラメータ。
	struct Particle
	{
		DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 acceleration = { 0.0f, 0.0f, 0.0f };
		float startSize = 1.0f;
		float endSize = 0.0f;
		float rotation = 0.0f;
		float angularVelocity = 0.0f;
		float damping = 0.0f;
		float age = 0.0f;
		float lifetime = 1.0f;
	};

	void Emit(const Particle& particle);

private:
	std::vector<Particle> m_Particles;
	std::vector<const Particle*> m_DrawOrder;
	Billboard* m_pBillboard;
	std::size_t m_MaxParticles;
};
