#include "camera.h"
#include "keyboard.h"
#include "mouse.h"
#include "scene.h"
#include "fade.h"

#include "gamecamera.h"

GameCamera* GameCamera::s_Instance=nullptr;

bool cameraIndex;

void GameCamera::Init() {
	s_Instance = new GameCamera();
	Camera_Initialize();
	LockMouse();
	s_Instance->m_Pos = { 0.0f,0.0f,0.0f };
	s_Instance->m_yaw = 0.0f;
	s_Instance->m_pitch = 0.0f;

	// カメラ角度の補間設定を初期化
	s_Instance->m_CurrentYaw = 0.0f;
	s_Instance->m_CurrentPitch = 15.0f; // 初期は少し上から見下ろす角度
	s_Instance->m_TargetYaw = 0.0f;
	s_Instance->m_TargetPitch = 15.0f;
	s_Instance->m_AngleLerpSpeed = 0.05f; // ゆっくり補間（値が小さいほど滑らか）

	cameraIndex = true;
}

void GameCamera::Update(Player* player) {
	if (Keyboard_IsKeyDownTrigger(KK_D4))cameraIndex= !cameraIndex;

	if (!cameraIndex)
	{
		const float SPEED = 0.1f;
		Mouse_State mouseState;
		Mouse_GetState(&mouseState);

		s_Instance->m_yaw += static_cast<float>(mouseState.dx) * 0.1f;
		s_Instance->m_pitch += static_cast<float>(mouseState.dy) * 0.1f;

		if (s_Instance->m_pitch > 89.0f) s_Instance->m_pitch = 89.0f;
		if (s_Instance->m_pitch < -89.0f) s_Instance->m_pitch = -89.0f;

		float yawRad = XMConvertToRadians(s_Instance->m_yaw);
		float pitchRad = XMConvertToRadians(s_Instance->m_pitch);

		XMVECTOR forward = XMVectorSet(sinf(yawRad), 0.0f, cosf(yawRad), 0.0f);
		XMVECTOR right = XMVectorSet(cosf(yawRad), 0.0f, -sinf(yawRad), 0.0f);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMVECTOR moveDir = XMVectorZero();

		if (Keyboard_IsKeyDown(KK_W)) moveDir = XMVectorAdd(moveDir, forward);
		if (Keyboard_IsKeyDown(KK_S)) moveDir = XMVectorSubtract(moveDir, forward);
		if (Keyboard_IsKeyDown(KK_D)) moveDir = XMVectorAdd(moveDir, right);
		if (Keyboard_IsKeyDown(KK_A)) moveDir = XMVectorSubtract(moveDir, right);

		if (Keyboard_IsKeyDown(KK_SPACE)) moveDir = XMVectorAdd(moveDir, up);
		if (Keyboard_IsKeyDown(KK_LEFTSHIFT) || Keyboard_IsKeyDown(KK_RIGHTSHIFT)) moveDir = XMVectorSubtract(moveDir, up);

		if (!XMVector3Equal(moveDir, XMVectorZero()))
		{
			moveDir = XMVector3Normalize(moveDir);
			moveDir = XMVectorScale(moveDir, SPEED);

			XMVECTOR pos = XMLoadFloat3(&s_Instance->m_Pos);
			pos = XMVectorAdd(pos, moveDir);
			XMStoreFloat3(&s_Instance->m_Pos, pos);
		}

		XMVECTOR lookDir = XMVectorSet(
			sinf(yawRad) * cosf(pitchRad),
			-sinf(pitchRad),
			cosf(yawRad) * cosf(pitchRad),
			0.0f
		);

		XMVECTOR posVec = XMLoadFloat3(&s_Instance->m_Pos);
		XMVECTOR atVec = XMVectorAdd(posVec, lookDir);

		XMFLOAT3 atPos;
		XMStoreFloat3(&atPos, atVec);

		if (GetCamera()) {
			GetCamera()->UpdateView(s_Instance->m_Pos, atPos);
		}
	}
	else {
		// プレイヤーの重力面に応じてカメラの目標角度を更新
		// 重力移動中なら目標の重力面を使用（即座にカメラが反応する）
		int targetGravityFace = player->IsGravityMoving() ? player->GetTargetFace() : player->GetGravityFace();
		s_Instance->UpdateCameraAngleByGravity(targetGravityFace);

		// 現在の角度を目標角度に向けてゆっくり補間
		s_Instance->m_CurrentYaw += (s_Instance->m_TargetYaw - s_Instance->m_CurrentYaw) * s_Instance->m_AngleLerpSpeed;
		s_Instance->m_CurrentPitch += (s_Instance->m_TargetPitch - s_Instance->m_CurrentPitch) * s_Instance->m_AngleLerpSpeed;

		// カメラ位置は固定（真ん中から少し後ろ）
		s_Instance->m_Pos = { 0.0f, 0.0f, -8.0f };

		// 固定された中央位置を基準に注視点を計算（プレイヤーの位置は使わない）
		XMFLOAT3 centerPos = { 0.0f, 0.0f, 0.0f };

		// 角度をラジアンに変換
		float yawRad = XMConvertToRadians(s_Instance->m_CurrentYaw);
		float pitchRad = XMConvertToRadians(s_Instance->m_CurrentPitch);

		// 中央位置から15度分オフセットした注視点を計算
		float offsetDistance = 1.0f; // オフセットの強さ
		XMFLOAT3 atPos;
		atPos.x = centerPos.x + offsetDistance * sinf(yawRad);
		atPos.y = centerPos.y + offsetDistance * sinf(pitchRad);
		atPos.z = centerPos.z;

		s_Instance->m_AtPos = atPos;

		if (GetCamera()) {
			GetCamera()->UpdateView(s_Instance->m_Pos, s_Instance->m_AtPos);
		}
	}

	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
	{
		SetSceneFade(SCENE_TITLE);
	}
}

void GameCamera::Draw() {

}

void GameCamera::Finalize() {
	Camera_Finalize();
}

void GameCamera::UpdateCameraAngleByGravity(int gravityFace)
{
	// 重力面に応じてカメラを15度程度だけ傾ける
	switch (gravityFace)
	{
	case FACE_FLOOR:
		// 床にいるとき：少し上から見下ろす
		m_TargetYaw = 0.0f;
		m_TargetPitch = 15.0f;
		break;

	case FACE_CEILING:
		// 天井にいるとき：少し下から見上げる
		m_TargetYaw = 0.0f;
		m_TargetPitch = -15.0f;
		break;

	case FACE_LEFT_WALL:
		// 左壁にいるとき：左から見る（角度を大きく）
		m_TargetYaw = -30.0f;
		m_TargetPitch = 0.0f;
		break;

	case FACE_RIGHT_WALL:
		// 右壁にいるとき：右から見る（角度を大きく）
		m_TargetYaw = 30.0f;
		m_TargetPitch = 0.0f;
		break;
	}
}