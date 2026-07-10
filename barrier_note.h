#pragma once
#include "note_base.h"

class BarrierNote : public NoteBase
{
public:
	BarrierNote() : NoteBase() {}

	void Init(int lane, int face, float spawnZ, float speed);
};