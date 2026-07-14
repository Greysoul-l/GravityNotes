#pragma once
#include "note_base.h"

class BarrierNote : public NoteBase
{
private:
	float m_Beat = 0.0f;
public:
	BarrierNote() : NoteBase() {}

	void Init(int lane, int face, float spawnZ, float speed, float beat);
	float GetBeat() const { return m_Beat; }

	void OnHit() override;
	void OnMiss() override;
};