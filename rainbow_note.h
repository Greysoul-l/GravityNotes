#pragma once
#include "note_base.h"
#include <d3d11.h>
#include <vector>

class RopeHoldNote : public NoteBase
{
public:
	enum class State { IDLE, HOLDING, COMPLETE, FAILED };

private:
	std::vector<int> m_FacePath; // 経由する面の並び（先頭=開始面、末尾=終了面）。90°ごとに1要素
	int   m_EndLane;
	float m_RopeLength;
	float m_HoldProgress;
	State m_State;

	ID3D11ShaderResourceView* m_Texture;
	float m_LoopTime;      // 30タイル1ループにかかる秒数
	float m_InitialSpawnZ; // タイルインデックス計算の基準Z

	// t(0~1) が属するセグメント（面ペア）とそのローカルtを求める
	void ResolveSegment(float t, int& faceA, int& faceB, float& localT) const;
	XMFLOAT2 EvalCurveXY(float t) const;
	XMFLOAT2 EvalNormal(float t) const;

public:
	RopeHoldNote()
		: NoteBase(), m_FacePath{ 0 }, m_EndLane(0),
		  m_RopeLength(0.0f), m_HoldProgress(0.0f), m_State(State::IDLE),
		  m_Texture(nullptr), m_LoopTime(2.0f), m_InitialSpawnZ(0.0f) {}

	NoteType GetType() const override { return NoteType::RopeHold; }

	static void FinalizeSharedResources();
	void SetLoopTime(float t) { m_LoopTime = t; }

	// facePath: 経由する全ての面（先頭=startFace、末尾=endFace）。要素数1なら回転なし
	void Init(int startLane, int endLane, const std::vector<int>& facePath,
	          float startZ, float endZ, float speed);
	void Update() override;
	void Draw()   override;
	void OnHit()  override; // Activate() に委譲

	bool Activate(); // IDLE → HOLDING
	void Release();  // 途中離し → FAILED
	void Complete(); // 終端通過 → COMPLETE

	State GetState()        const { return m_State; }
	int   GetEndFace()      const { return m_FacePath.back(); }
	int   GetEndLane()      const { return m_EndLane; }
	float GetHoldProgress() const { return m_HoldProgress; }

	XMFLOAT2 GetCurveXY(float t) const { return EvalCurveXY(t); }
	// プレイヤーの向き補間用：tが属するセグメントの両端面とローカルtを返す
	void GetSegmentInfo(float t, int& faceA, int& faceB, float& localT) const { ResolveSegment(t, faceA, faceB, localT); }
};
