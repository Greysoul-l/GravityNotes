#pragma once
#include "enemy_note.h"
#include <vector>

// ======================================================
// HoldNote: EnemyNote を一定間隔で並べた長押しノーツ
// 子ノートの間隔は hold_note.cpp の HOLD_BEAT_INTERVAL で変更可能
// ======================================================
class HoldNote : public NoteBase
{
private:
	std::vector<EnemyNote*> m_ChildNotes;

public:
	HoldNote() : NoteBase() {}
	~HoldNote();

	NoteType GetType() const override { return NoteType::Hold; }

	// lane    : 始点レーン (0-2)
	// endLane : 終点レーン (0-2)
	// face    : 壁面 (FACE_*)
	// initZ   : 始点の初期Z座標
	// endZ    : 終点の初期Z座標
	// speed   : ノーツ速度
	// bpm     : 曲のBPM
	void Init(int lane, int endLane, int face, float initZ, float endZ, float speed, float bpm, float beat, float endBeat);
	void Update() override;
	void Draw()   override;

	// lane・face が一致する最も手前の未ヒット子ノートを返す（なければ nullptr）
	EnemyNote* GetNearestActiveChild(int lane, int face) const;
};
