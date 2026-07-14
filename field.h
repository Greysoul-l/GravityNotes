#pragma once
#include "renderer.h"
#include "sprite3d.h"

class Field: public Sprite3D
{
private:
	static constexpr int NUM_FIELDS = 20;
	static constexpr float SHADOW_LOD_NEAR = 30.0f; // 影を描画するLOD閾値
	float m_ScrollPos[NUM_FIELDS];
	MODEL* m_FieldModels[NUM_FIELDS];

	MODEL* m_ModelNormal = nullptr;
	MODEL* m_ModelHasiranashi = nullptr;

public:
	Field() : Sprite3D() {}
    void Init();
    void Update(float speed);
    void Draw();
    void Finalize();
};
