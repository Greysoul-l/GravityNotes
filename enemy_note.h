#pragma once
#include "note_base.h"

class EnemyNote : public NoteBase
{
public:
	EnemyNote() : NoteBase() {}

	// modelPath : nullptrの場合は既定の "asset/model/Gargoyle.fbx" を使用
	void Init(int lane, int face, float spawnZ, float speed, const char* modelPath = nullptr);
};