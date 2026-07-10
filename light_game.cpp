#include "light_game.h"

#include "../framework/imgui/imgui.h"

AmbientLight GameLight::m_ambientLight(XMFLOAT4(0.55f, 0.55f, 0.55f, 1.0f));
PointLight* GameLight::m_pointLight = nullptr;

void GameLight::Init()
{
	m_pointLight = new PointLight(
		TRUE,
		{ 0.0f, 4.0f, -4.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		40.0f,
		1.0f
	);

	m_pointLight->Apply(m_ambientLight);
}

void GameLight::Finalize()
{
	SAFE_DELETE(m_pointLight);
}

void GameLight::Update()
{
	//if (m_pointLight) m_pointLight->Apply(m_ambientLight);
}

void GameLight::Draw()
{
#ifdef _DEBUG
	if (!m_pointLight) return;

	XMFLOAT4 lightPosition = m_pointLight->GetPosition();
	XMFLOAT4 lightDiffuse = m_pointLight->GetDiffuse();
	XMFLOAT4 ambientColor = m_ambientLight.GetColor();
	float lightRange = m_pointLight->GetRange();
	float lightIntensity = m_pointLight->GetIntensity();

	//ライトのパラメータ
	/*ImGui::Begin("Game PBR Light");
	ImGui::DragFloat3("Light Pos", &lightPosition.x, 0.1f, -50.0f, 50.0f);
	ImGui::SliderFloat("Light Range", &lightRange, 1.0f, 100.0f);
	ImGui::SliderFloat("Light Intensity", &lightIntensity, 0.0f, 5.0f);
	ImGui::ColorEdit3("Light Color", &lightDiffuse.x);
	ImGui::ColorEdit3("Ambient", &ambientColor.x);
	ImGui::End();*/

	m_pointLight->SetPosition(lightPosition);
	m_pointLight->SetDiffuse(lightDiffuse);
	m_pointLight->SetRange(lightRange);
	m_pointLight->SetIntensity(lightIntensity);
	m_ambientLight.SetColor(ambientColor);
	m_pointLight->Apply(m_ambientLight);
#endif
}
