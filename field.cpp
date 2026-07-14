#include "field.h"
#include "shadermanager.h"
#include "define.h"
#include "game.h"
#include <float.h>

using namespace DirectX;

void Field::Init() {
	m_Scale = { 5.0f,5.0f,5.0f };
	m_ModelNormal = ModelLoad("asset/model/field_allnormal.fbx");
	m_ModelHasiranashi = ModelLoad("asset/model/field_hasiranashi.fbx");
	m_Model = m_ModelHasiranashi;
	m_ShaderType = S_PHONG_SHADOW; // Phongライティング + ShadowMapの落ち影を受ける

	// Sprite3Dのメンバ変数を初期化
	m_ModelSize = ModelGetSize(m_ModelNormal);
	m_OriginalColor = ModelGetAverageMaterialColor(m_ModelNormal);

	float L = GetDisplaySize().z;
	if (L <= 0.1f) {
		L = 20.0f; // フォールバック値
	}

	for (int i = 0; i < NUM_FIELDS; ++i) {
		m_ScrollPos[i] = (float)i * L;
		if (i % 2 == 0) {
			m_FieldModels[i] = m_ModelNormal;
		} else {
			m_FieldModels[i] = m_ModelHasiranashi;
		}
	}
}

void Field::Update(float speed){
	float L = GetDisplaySize().z;
	if (L <= 0.1f) {
		L = 20.0f; // フォールバック値
	}

	// すべてのフィールドを手前にスクロール
	for (int i = 0; i < NUM_FIELDS; ++i) {
		m_ScrollPos[i] -= speed * dt;
	}

	// 画角から見切れたらまた奥に戻す
	// カメラのZ座標は -8.0f。手前見切れ境界を -10.0f とする。
	float limitZ = -10.0f;
	for (int i = 0; i < NUM_FIELDS; ++i) {
		if (m_ScrollPos[i] + L / 2.0f < limitZ) {
			// 最も奥にあるモデルのZ座標を探す
			float maxZ = -FLT_MAX;
			for (int j = 0; j < NUM_FIELDS; ++j) {
				if (m_ScrollPos[j] > maxZ) {
					maxZ = m_ScrollPos[j];
				}
			}
			// その奥に連結して配置
			m_ScrollPos[i] = maxZ + L;
		}
	}
}

void Field::Draw() {
	XMFLOAT3 originalPos = GetPos();

	// 1パス目: ShadowMap影受けあり (S_PHONG_SHADOW) のセグメントを描画
	m_ShaderType = S_PHONG_SHADOW;
	for (int i = 0; i < NUM_FIELDS; ++i) {
		if (m_ScrollPos[i] <= SHADOW_LOD_NEAR) {
			XMFLOAT3 tempPos = originalPos;
			tempPos.z = m_ScrollPos[i];
			SetPos(tempPos);
			m_Model = m_FieldModels[i];
			Sprite3D::Draw();
		}
	}

	// 2パス目: 通常ライティングのみ (S_PHONG) のセグメントを描画
	m_ShaderType = S_PHONG;
	for (int i = 0; i < NUM_FIELDS; ++i) {
		if (m_ScrollPos[i] > SHADOW_LOD_NEAR) {
			XMFLOAT3 tempPos = originalPos;
			tempPos.z = m_ScrollPos[i];
			SetPos(tempPos);
			m_Model = m_FieldModels[i];
			Sprite3D::Draw();
		}
	}

	SetPos(originalPos);
}

void Field::Finalize() {
	if (m_ModelNormal) {
		ModelRelease(m_ModelNormal);
		m_ModelNormal = nullptr;
	}
	if (m_ModelHasiranashi) {
		ModelRelease(m_ModelHasiranashi);
		m_ModelHasiranashi = nullptr;
	}
	m_Model = nullptr; // Sprite3Dの二重解放防止
}