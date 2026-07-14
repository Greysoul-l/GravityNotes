#include "game.h"
#include "orb_note.h"
#include "split_bilboard.h"
#include <cstdlib>

namespace
{
	const int   ORB_SHEET_COLS   = 5;
	const int   ORB_SHEET_ROWS   = 11;
	const int   ORB_USABLE_FRAMES = 54; // グリッドは55マスだが最後の1マスは未使用
	const float ORB_ANIM_FPS     = 18.0f;
	const XMFLOAT2 ORB_BILLBOARD_SIZE = { 2.0f, 2.0f };
	const float ORB_DRAW_Y_OFFSET    = 1.0f;
	const float ORB_SHADOW_FORWARD_OFFSET = 1.5f; // 影だけプレイヤー進行方向(+Z)へずらす
}

OrbNote::~OrbNote()
{
	if (m_pBillboard)
	{
		delete m_pBillboard;
		m_pBillboard = nullptr;
	}
}

void OrbNote::Init(int lane, int face, float spawnZ, float speed)
{
	// fbxモデルは使用しないため modelPath は渡さない
	NoteBase::Init(lane, face, spawnZ, speed, nullptr);

	m_AnimTimer = (static_cast<float>(rand()) / RAND_MAX) * (1.0f / ORB_ANIM_FPS);

	if (!m_pBillboard)
	{
		XMFLOAT3 drawPos = GetOffsetPos();
		m_pBillboard = new SplitBilBoard(
			ORB_SHEET_COLS, ORB_SHEET_ROWS,
			drawPos, ORB_BILLBOARD_SIZE, { 0.0f, 0.0f, 0.0f },
			"asset/texture/OrbAnimationSpriteSheetRed.png",
			false
		);
		m_pBillboard->SetBillboardMode(true);
		m_pBillboard->SetWallFadeEnabled(false);
	}
	m_pBillboard->SetPos(GetOffsetPos());
	m_pBillboard->SetTextureIndex(rand() % ORB_USABLE_FRAMES);
}

void OrbNote::Update()
{
	NoteBase::Update();

	if (m_pBillboard)
	{
		m_pBillboard->SetPos(GetOffsetPos());

		// SplitBilBoardの自動アニメーションはグリッド全体(55駒)を回してしまうため、
		// 使用駒数(54駒)だけを手動でループさせる
		m_AnimTimer += dt;
		float interval = 1.0f / ORB_ANIM_FPS;
		if (m_AnimTimer >= interval)
		{
			m_AnimTimer -= interval;
			int frame = (m_pBillboard->GetTextureIndex() + 1) % ORB_USABLE_FRAMES;
			m_pBillboard->SetTextureIndex(frame);
		}
	}
}

void OrbNote::Draw()
{
	if (m_pBillboard)
		m_pBillboard->Draw();
}

void OrbNote::DrawShadowMap(const XMMATRIX& lightView, const XMMATRIX& lightProjection)
{
	if (m_pBillboard)
	{
		XMFLOAT3 originalPos = m_pBillboard->GetPos();
		XMFLOAT3 shadowPos = originalPos;
		shadowPos.z += ORB_SHADOW_FORWARD_OFFSET;
		m_pBillboard->SetPos(shadowPos);
		m_pBillboard->DrawShadowMap(lightView, lightProjection);
		m_pBillboard->SetPos(originalPos);
	}
}

float OrbNote::GetDrawDepth(const XMMATRIX& view) const
{
	XMFLOAT3 drawPos = GetOffsetPos();
	XMVECTOR viewPos = XMVector3TransformCoord(XMLoadFloat3(&drawPos), view);
	return XMVectorGetZ(viewPos);
}

XMFLOAT3 OrbNote::GetOffsetPos() const
{
	XMFLOAT3 pos = GetPos();
	int face = GetFace();
	if (face == 0)      pos.y += ORB_DRAW_Y_OFFSET; // 床
	else if (face == 1) pos.x += ORB_DRAW_Y_OFFSET; // 左壁
	else if (face == 2) pos.y -= ORB_DRAW_Y_OFFSET; // 天井
	else if (face == 3) pos.x -= ORB_DRAW_Y_OFFSET; // 右壁
	return pos;
}
