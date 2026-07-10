#pragma once
#include "renderer.h"
#include "anim_sprite3d.h"
#include "mathhelper.h"
#include "split_bilboard.h"

class NoteManager;    // 前方宣言
class StatusManager;  // 前方宣言

enum LANE {
	LANE_LEFT = -1,	//=DOWN
	LANE_CENTER,
	LANE_RIGHT			//=UP
};

enum FACE {
	FACE_FLOOR = 0,
	FACE_LEFT_WALL,
	FACE_CEILING,
	FACE_RIGHT_WALL
};

class Player :public AnimSprite3D
{
private:
	int m_LaneIndex;//現在のレーン
	int m_TargetLaneIndex;//移動先のレーン（補完用）
	int m_GravityFace;//現在いる面
	int m_TargetFace;//移動先の面（補完用）
	XMFLOAT3 m_TargetPos;//移動先の３D座標
	XMFLOAT3 m_TargetRot;//移動先の回転角
	bool m_IsMoving;//補完長宇迦堂かっもフラグ（二重移動防止、アニメーション分岐用）
	float m_MoveTimer;// 0.0f〜移動にかかる時間（秒）
	float m_MoveDuration; // 移動1回にかける総時間（例: 0.1f〜0.15f）
	XMFLOAT3 m_StartPos;// 移動開始時の座標
	bool m_IsGravityMoving;
	float m_GravityTimer;
	float m_GravityDuration;
	XMFLOAT3 m_GravityStartPos;
	XMFLOAT3 m_GravityStartRot;

	NoteManager*   m_pNoteManager;
	StatusManager* m_pStatusManager;
	SplitBilBoard* m_pEffectSlash;
	bool           m_IsEffectSlashActive;
	bool           m_IsOverridePlaying;

public:
	// デフォルトコンストラクタを追加し、基底 Sprite3D のコンストラクタを呼ぶ
	Player()
		: AnimSprite3D(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f),
			"asset/model/knight_02.fbx", S_PBR)
	{}
	void Init(NoteManager* nm, StatusManager* sm);
	void Update();
	void Draw();
	void Finalize();

	int GetLaneIndex()   const { return m_LaneIndex; }
	int GetGravityFace() const { return m_GravityFace; }
	int GetTargetFace() const { return m_TargetFace; }
	bool IsGravityMoving() const { return m_IsGravityMoving; }

	void MoveLeft();
	void MoveRight();
	void ChangeGravity(int targetFace);
	XMFLOAT3 CalcLaneTargetPos(int laneIndex);
	XMFLOAT3 CalcFaceTargetPos(int face, int laneIndex);
	XMFLOAT3 CalcFaceTargetRot(int face);
	int CalcNearestLane();
};