#pragma once
#include "note_base.h"
#include <d3d11.h>

class RopeHoldNote : public NoteBase
{
public:
	enum class State { IDLE, HOLDING, COMPLETE, FAILED };

private:
	int   m_EndFace;
	int   m_EndLane;
	float m_RopeLength;
	float m_HoldProgress;
	State m_State;

	ID3D11ShaderResourceView* m_Texture;
	float m_LoopTime;      // 30タイル1ループにかかる秒数
	float m_InitialSpawnZ; // タイルインデックス計算の基準Z

public:
	RopeHoldNote()
		: NoteBase(), m_EndFace(0), m_EndLane(0),
		  m_RopeLength(0.0f), m_HoldProgress(0.0f), m_State(State::IDLE),
		  m_Texture(nullptr), m_LoopTime(2.0f), m_InitialSpawnZ(0.0f) {}

	void SetLoopTime(float t) { m_LoopTime = t; }

	void Init(int startLane, int endLane, int startFace, int endFace,
	          float startZ, float endZ, float speed);
	void Update() override;
	void Draw()   override;
	void OnHit()  override; // Activate() に委譲

	bool Activate(); // IDLE → HOLDING
	void Release();  // 途中離し → FAILED
	void Complete(); // 終端通過 → COMPLETE

	State GetState()        const { return m_State; }
	int   GetEndFace()      const { return m_EndFace; }
	int   GetEndLane()      const { return m_EndLane; }
	float GetHoldProgress() const { return m_HoldProgress; }

	XMFLOAT2 GetCurveXY(float t) const;
};
