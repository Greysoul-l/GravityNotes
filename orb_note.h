#pragma once
#include "note_base.h"

class SplitBilBoard; // 前方宣言

class OrbNote : public NoteBase
{
private:
	SplitBilBoard* m_pBillboard;
	float m_AnimTimer;

	XMFLOAT3 GetOffsetPos() const;

public:
	OrbNote() : NoteBase(), m_pBillboard(nullptr), m_AnimTimer(0.0f) {}
	~OrbNote();

	void Init(int lane, int face, float spawnZ, float speed);
	void Update() override;
	void Draw() override;
	void DrawShadowMap(const XMMATRIX& lightView, const XMMATRIX& lightProjection) override;
	float GetDrawDepth(const XMMATRIX& view) const;
	XMFLOAT3 GetEffectPosition() const { return GetOffsetPos(); }
};
