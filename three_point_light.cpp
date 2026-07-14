#include "three_point_light.h"

void ThreePointLight::Init()
{
	// キー：右手前・やや上から白く強め
	m_Key.enable    = TRUE;
	m_Key.offset    = XMFLOAT3( 3.0f, 2.5f, -2.1f);
	m_Key.color     = XMFLOAT4( 1.0f, 0.98f, 0.92f, 1.0f);
	m_Key.range     = 40.0f;
	m_Key.intensity = 3.2f;
	m_Key.specular  = 1.0f;   // 主光源なので反射あり

	// フィル：左手前からやや青白く弱め（影を軽く起こす）
	m_Fill.enable    = TRUE;
	m_Fill.offset    = XMFLOAT3(-3.0f, 2.5f, -2.1f);
	m_Fill.color     = XMFLOAT4( 0.8f, 0.85f, 1.0f, 1.0f);
	m_Fill.range     = 40.0f;
	m_Fill.intensity = 2.0f;
	m_Fill.specular  = 0.0f;   // 暗部を起こす役割なので反射なし

	// リム：輪郭を出す
	m_Rim.enable    = TRUE;
	m_Rim.offset    = XMFLOAT3( 2.2f, -1.3f, 1.0f);
	m_Rim.color     = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f);
	m_Rim.range     = 40.0f;
	m_Rim.intensity = 3.2f;
	m_Rim.specular   = 0.0f;
}

LIGHT ThreePointLight::MakeLight(const ThreePointLightParam& p, const XMFLOAT3& targetPos, const XMMATRIX& offsetRot) const
{
	LIGHT light = {};
	light.Enable = p.enable;
	light.Diffuse = p.color;
	// オフセットを対象の回転に合わせて回してから、対象位置に足す。
	// これで体に対して常に同じ方向からライトが当たる。
	XMVECTOR offset = XMVectorSet(p.offset.x, p.offset.y, p.offset.z, 0.0f);
	offset = XMVector3TransformNormal(offset, offsetRot);
	XMFLOAT3 o;
	XMStoreFloat3(&o, offset);
	light.Position = XMFLOAT4(
		targetPos.x + o.x,
		targetPos.y + o.y,
		targetPos.z + o.z, 1.0f);
	// z にスペキュラー強度を入れる（PBRシェーダー側で参照）
	// 全体倍率(m_Specular) × ライト個別倍率(p.specular)
	light.PointLightParam = XMFLOAT4(p.range, p.intensity, m_Specular * p.specular, 0.0f);
	return light;
}

void ThreePointLight::Apply(const XMFLOAT3& targetPos, const XMFLOAT3& targetRot)
{
	// オフセットの基準は「デフォルト向き(Y=180°でカメラを向いた状態)」で調整してある。
	// 現在の回転との差分だけオフセットを回すことで、
	//   ・デフォルト向き … 調整した見た目そのまま
	//   ・重力面などで回転 … 体に対して同じ当たり方
	// になる。
	const XMFLOAT3 kDefaultRot = XMFLOAT3(0.0f, 180.0f, 0.0f);
	XMMATRIX matDefault = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(kDefaultRot.x),
		XMConvertToRadians(kDefaultRot.y),
		XMConvertToRadians(kDefaultRot.z));
	XMMATRIX matCurrent = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(targetRot.x),
		XMConvertToRadians(targetRot.y),
		XMConvertToRadians(targetRot.z));
	// R = M_current * M_default^-1（回転行列なので逆行列は転置）
	XMMATRIX offsetRot = XMMatrixMultiply(XMMatrixTranspose(matDefault), matCurrent);

	// 3灯をまとめてPBRシェーダー(b7)へ送る
	LIGHT lights[NUM_PLAYER_LIGHTS];
	lights[0] = MakeLight(m_Key,  targetPos, offsetRot);
	lights[1] = MakeLight(m_Fill, targetPos, offsetRot);
	lights[2] = MakeLight(m_Rim,  targetPos, offsetRot);
	// 先頭ライトのAmbientにこのモデル専用の環境光を載せる（PBRシェーダーが参照）
	lights[0].Ambient = m_Ambient;
	SetPlayerLights(lights);
}
