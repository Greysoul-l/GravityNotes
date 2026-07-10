#include "Common.hlsl"

Texture2D g_Texture : register(t0);
Texture2D g_NormalMap : register(t2);
Texture2D g_MetallicMap : register(t3);
Texture2D g_RoughnessMap : register(t4);
Texture2D g_EmissiveMap : register(t5);
SamplerState g_SamplerState : register(s0);

static const float PI = 3.14159265f;

float3 CalcNormalMapWorldNormal(PS_IN In)
{
	float3 baseNormal = normalize(In.Normal.xyz);
	float3 normalSample = g_NormalMap.Sample(g_SamplerState, In.TexCoord).xyz * 2.0f - 1.0f;

	float3 dp1 = ddx(In.WorldPosition.xyz);
	float3 dp2 = ddy(In.WorldPosition.xyz);
	float2 duv1 = ddx(In.TexCoord);
	float2 duv2 = ddy(In.TexCoord);

	float det = duv1.x * duv2.y - duv1.y * duv2.x;
	float3 tangent;
	float3 bitangent;
	if (abs(det) > 0.00000001f)
	{
		float invDet = 1.0f / det;
		tangent = (dp1 * duv2.y - dp2 * duv1.y) * invDet;
		bitangent = (dp2 * duv1.x - dp1 * duv2.x) * invDet;
	}
	else
	{
		float3 axis = (abs(baseNormal.y) < 0.99f) ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f);
		tangent = cross(axis, baseNormal);
		bitangent = cross(baseNormal, tangent);
	}

	tangent = normalize(tangent);
	bitangent = normalize(bitangent);
	return normalize(normalSample.x * tangent + normalSample.y * bitangent + normalSample.z * baseNormal);
}

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
	float4 texColor = g_Texture.Sample(g_SamplerState, In.TexCoord);
	// PBR 用の追加テクスチャから金属度・粗さ・自己発光を読む。
	float3 emissive = g_EmissiveMap.Sample(g_SamplerState, In.TexCoord).rgb;
	float3 albedo = texColor.rgb * In.Diffuse.rgb;
	float metallic = saturate(g_MetallicMap.Sample(g_SamplerState, In.TexCoord).r);
	float roughness = saturate(g_RoughnessMap.Sample(g_SamplerState, In.TexCoord).r);
	roughness = max(roughness, 0.04f);

	float3 N = CalcNormalMapWorldNormal(In);
	float3 V = normalize(CameraPosition.xyz - In.WorldPosition.xyz);
	float3 L = normalize(Light.Position.xyz - In.WorldPosition.xyz);
	float3 H = normalize(V + L);

	float distanceToLight = length(Light.Position.xyz - In.WorldPosition.xyz);
	float lightRange = max(Light.PointLightParam.x, 0.001f);
	float lightIntensity = max(Light.PointLightParam.y, 0.0f);
	float attenuation = saturate(1.0f - distanceToLight / lightRange);
	attenuation = attenuation * attenuation * lightIntensity;

	float NdotL = Light.Enable ? saturate(dot(N, L)) * attenuation : 0.0f;
	float NdotV = max(dot(N, V), 0.001f);
	float NdotH = max(dot(N, H), 0.001f);
	float VdotH = max(dot(V, H), 0.001f);

	// GGX ベースの簡易 PBR 反射計算。
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
	float D = a2 / max(PI * denom * denom, 0.001f);

	float k = (roughness + 1.0f);
	k = (k * k) / 8.0f;
	float Gv = NdotV / (NdotV * (1.0f - k) + k);
	float Gl = NdotL / (NdotL * (1.0f - k) + k);
	float G = Gv * Gl;

	float3 F0 = lerp(0.04f.xxx, albedo, metallic);
	float3 F = F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);
	float3 specular = (D * G * F) / max(4.0f * NdotV * max(NdotL, 0.001f), 0.001f);
	float3 diffuse = albedo * (1.0f - metallic) / PI;

	float3 ambient = albedo * saturate(Light.Ambient.rgb);
	float3 direct = (diffuse + specular) * Light.Diffuse.rgb * NdotL;
	outDiffuse.rgb = saturate(ambient + direct + emissive);
	outDiffuse.a = texColor.a * In.Diffuse.a;
}
