#pragma once
#include "sprite3d.h"

class NoteBase : public Sprite3D
{
protected:
	int   m_LaneIndex;
	int   m_Face;
	float m_Speed;
	bool  m_IsActive;
	bool  m_IsHit;

public:
	NoteBase() : Sprite3D(), m_LaneIndex(0), m_Face(0), m_Speed(0.0f), m_IsActive(false), m_IsHit(false) {}
	virtual ~NoteBase() = default;

	virtual void Init(int lane, int face, float spawnZ, float speed, const char* modelPath);
	virtual void Update();
	virtual void Draw();
	virtual void Finalize();
	virtual void OnHit();
	virtual void OnMiss();

	bool IsActive() const { return m_IsActive; }
	bool IsHit()    const { return m_IsHit; }
	int  GetLaneIndex() const { return m_LaneIndex; }
	int  GetFace()      const { return m_Face; }
};

// 面をまたいだ隣接レーンの角判定
// (Floor LANE_LEFT<->LeftWall LANE_LEFT / Floor LANE_RIGHT<->RightWall LANE_LEFT /
//  Ceiling LANE_LEFT<->LeftWall LANE_RIGHT / Ceiling LANE_RIGHT<->RightWall LANE_RIGHT)
// face: 0=FLOOR, 1=LEFT_WALL, 2=CEILING, 3=RIGHT_WALL / lane: -1=LEFT, 0=CENTER, 1=RIGHT
bool IsCornerAdjacent(int lane1, int face1, int lane2, int face2);

// 完全一致、または隣接面の角どうしなら true（Enemy・Hold連撃など、角越しのヒットを許容したい判定に使う）
bool IsSameOrCornerPosition(int lane1, int face1, int lane2, int face2);