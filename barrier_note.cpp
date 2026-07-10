#include "game.h"
#include "barrier_note.h"

void BarrierNote::Init(int lane, int face, float spawnZ, float speed)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/barrier.fbx");
	SetSize(XMFLOAT3(1.4f, 1.4f, 1.4f));
	m_ShaderType = S_LAMBERT;
	SetColor(1.0f, 1.0f, 1.0f);
}