#include "game.h"
#include "enemy_note.h"

void EnemyNote::Init(int lane, int face, float spawnZ, float speed, const char* modelPath)
{
	NoteBase::Init(lane, face, spawnZ, speed, modelPath ? modelPath : "asset/model/Gargoyle.fbx");
	m_Scale = { 0.03f,0.03f,0.03f };
	m_ShaderType = S_LAMBERT;
}

