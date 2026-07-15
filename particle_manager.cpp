#include "particle_manager.h"

#include <algorithm>
#include "billboard.h"
#include "camera.h"

using namespace DirectX;

ParticleManager::ParticleManager(const char* texturePath, std::size_t maxParticles)
	: m_pBillboard(new Billboard(
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		XMFLOAT2(1.0f, 1.0f),
		XMFLOAT3(0.0f, 0.0f, 0.0f),
		texturePath,
		false)),
	  m_MaxParticles(maxParticles)
{
	m_Particles.reserve(maxParticles);
	m_DrawOrder.reserve(maxParticles);
	m_pBillboard->SetBillboardMode(true);
	m_pBillboard->SetWallFadeEnabled(false);
}

ParticleManager::~ParticleManager()
{
	delete m_pBillboard;
	m_pBillboard = nullptr;
}

void ParticleManager::Emit(const Particle& particle)
{
	if (particle.lifetime <= 0.0f || m_MaxParticles == 0)
		return;

	if (m_Particles.size() >= m_MaxParticles)
	{
		// 上限時は最も古い粒子（インデックス0）を末尾の粒子で上書きして末尾を消すことで、O(1)で要素数を減らし、O(N)シフトを避ける
		m_Particles[0] = m_Particles.back();
		m_Particles.pop_back();
	}

	m_Particles.push_back(particle);
}

void ParticleManager::Update(float deltaTime)
{
	// 派生クラスから渡された物理パラメータだけで共通更新する。
	for (Particle& particle : m_Particles)
	{
		particle.age += deltaTime;
		particle.velocity.x += particle.acceleration.x * deltaTime;
		particle.velocity.y += particle.acceleration.y * deltaTime;
		particle.velocity.z += particle.acceleration.z * deltaTime;

		const float damping = (std::max)(0.0f, 1.0f - particle.damping * deltaTime);
		particle.velocity.x *= damping;
		particle.velocity.y *= damping;
		particle.velocity.z *= damping;

		particle.position.x += particle.velocity.x * deltaTime;
		particle.position.y += particle.velocity.y * deltaTime;
		particle.position.z += particle.velocity.z * deltaTime;
		particle.rotation += particle.angularVelocity * deltaTime;
	}

	m_Particles.erase(
		std::remove_if(
			m_Particles.begin(),
			m_Particles.end(),
			[](const Particle& particle) { return particle.age >= particle.lifetime; }),
		m_Particles.end());
}

void ParticleManager::Draw()
{
	m_DrawOrder.clear();
	for (const Particle& particle : m_Particles)
		m_DrawOrder.push_back(&particle);

	const XMMATRIX view = GetCamera()->GetView();
	std::stable_sort(
		m_DrawOrder.begin(),
		m_DrawOrder.end(),
		[&view](const Particle* lhs, const Particle* rhs)
		{
			XMVECTOR lhsViewPos = XMVector3TransformCoord(XMLoadFloat3(&lhs->position), view);
			XMVECTOR rhsViewPos = XMVector3TransformCoord(XMLoadFloat3(&rhs->position), view);
			return XMVectorGetZ(lhsViewPos) > XMVectorGetZ(rhsViewPos);
		});

	// 奥から手前へ描きつつ、1枚のBillboardを全粒子で使い回す。
	for (const Particle* particle : m_DrawOrder)
	{
		const float progress = (std::min)(particle->age / particle->lifetime, 1.0f);
		const float size = particle->startSize + (particle->endSize - particle->startSize) * progress;

		m_pBillboard->SetPos(particle->position);
		m_pBillboard->SetSize({ size, size });
		m_pBillboard->SetRotation({ 0.0f, 0.0f, particle->rotation });
		m_pBillboard->Draw();
	}
}
