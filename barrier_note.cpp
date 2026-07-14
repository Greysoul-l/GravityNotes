#include "game.h"
#include "barrier_note.h"

void BarrierNote::Init(int lane, int face, float spawnZ, float speed, float beat)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/barrier.fbx");
	SetSize(XMFLOAT3(1.4f, 1.4f, 1.4f));
	m_ShaderType = S_LAMBERT;
	SetColor(1.0f, 1.0f, 1.0f);
	m_Beat = beat;
}

void BarrierNote::OnHit()
{
	m_IsHit = true;
	// バリアノーツは判定が済んでもカメラ後方に下がるまで表示を残すため m_IsActive は変更しない
}

void BarrierNote::OnMiss()
{
	m_IsHit = true; // 被弾時も同様に表示を残す
}