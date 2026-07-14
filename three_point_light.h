#pragma once
#include "framework/light.h"

// =============================================================
// キャラクター用の3点照明（キー / フィル / リム）
//   ・キーライト : 右手前から強く当てる主光源
//   ・フィルライト: 左手前から弱く当てて影を和らげる
//   ・リムライト : 前から当てて輪郭を出す
//   各ライトは Position を持つポイントライトで、対象(プレイヤー)の
//   位置を基準に配置する。PBRシェーダー(S_PBR)にのみ適用される。
// =============================================================

// 1灯分の調整パラメータ（ImGui から直接いじれるように公開メンバで持つ）
struct ThreePointLightParam
{
	BOOL     enable;     // 有効フラグ
	XMFLOAT3 offset;     // 対象からの相対位置 X:右+/左- Y:上+/下- Z:手前(カメラ側)-/奥+
	XMFLOAT4 color;      // 色(RGB)
	float    range;      // 届く距離
	float    intensity;  // 強さ
	float    specular;   // このライトの反射(鏡面)の強さ倍率。0で反射なし
};

class ThreePointLight
{
public:
	void Init();

	// targetPos(足元位置) と targetRot(度) を基準に3灯を配置し、PBRシェーダーへ送る。
	// オフセットは対象の回転に追従するので、移動・重力面変更で向きが変わっても
	// 体に対して常に同じ方向から光が当たる。
	// 毎フレーム、Player の描画直前に呼ぶ。
	void Apply(const XMFLOAT3& targetPos, const XMFLOAT3& targetRot);

	// --- 各ライトのパラメータへの参照（ImGui などで直接編集する用） ---
	ThreePointLightParam& Key()  { return m_Key; }
	ThreePointLightParam& Fill() { return m_Fill; }
	ThreePointLightParam& Rim()  { return m_Rim; }

	// PBRスペキュラー(鏡面反射)の強さ倍率。3灯共通。
	float& Specular() { return m_Specular; }

	// このモデル(Player)専用のアンビエント(環境光)。
	// Fieldなど他モデルの Light.Ambient とは独立して設定できる。
	XMFLOAT4& Ambient() { return m_Ambient; }

private:
	LIGHT MakeLight(const ThreePointLightParam& p, const XMFLOAT3& targetPos, const XMMATRIX& offsetRot) const;

	ThreePointLightParam m_Key;
	ThreePointLightParam m_Fill;
	ThreePointLightParam m_Rim;

	float m_Specular = 1.0f;   // スペキュラー強度倍率
	XMFLOAT4 m_Ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f); // Player専用アンビエント
};
