// 水面用のシェーダー(WaterRenderer)。波の形は頂点シェーダーで時間から作り、法線もピクセルごとに同じ式の傾きから求める。
// 光(平行光源・点光源・スポット)と影の定数バッファは Shader3DCommon.hlsli と同じものを読む(b0, b3, b4, b5, t1, t2, s1)。
// 水面だけの設定は b6(WaterBuffer)に入る。C++ 側は Engine/WaterRenderer.h の WaterConstantBuffer(同じ並び)
// ※光の計算(点光源・スポットのループ)は Shader3DCommon.hlsli の ShadePixel と同じ式を、水面用に書き直している。そちらの式を変えた時は、ここも見直す
#include "Shader3DCommon.hlsli"

#define WATER_WAVE_COUNT 4   // 波の本数(WaterRenderer.h の waterWaveCount と同じ)

cbuffer WaterBuffer : register(b6)
{
    float4 waterColor;                // rgb=水の色, a=透明度(0=透明, 1=不透明)
    float4 waterSky;                  // rgb=空の色(浅い角度で映る色), w=影を受ける(1/0)
    float4 waterParams;               // x=specularStrength, y=fresnelPower, z=shininess
    float4 waterSize;                 // x=水面の幅(X), y=奥行き(Z)
    float4 waves[WATER_WAVE_COUNT];   // xy=波の進む向き(長さ1), z=振幅, w=周波数(2π ÷ 波長)
    float4 wavePhases;                // 波ごとの位相(0〜2π。時間から C++ 側が求めて送る。何時間遊んでも sin の精度が落ちない)
};

// 波の高さ h と、高さの傾き grad(x方向とz方向の微分)を、ワールドのxzから求める。
// h = Σ 振幅 × sin(向き・位置 × 周波数 − 位相)。C++ の WaterRenderer.cpp の EvalWaves と同じ式(GetHeightAt が使う)
void EvalWaves(float2 xz, out float h, out float2 grad)
{
    h = 0.0f;
    grad = float2(0.0f, 0.0f);
    [unroll]
    for (int i = 0; i < WATER_WAVE_COUNT; i++)
    {
        float4 w = waves[i];
        float phase = dot(w.xy, xz) * w.w - wavePhases[i];
        float s, c;
        sincos(phase, s, c);
        h += w.z * s;
        grad += w.xy * (w.z * w.w * c);
    }
}

struct WaterVSInput
{
    float2 grid : POSITION;   // 格子の位置(-0.5〜0.5)。大きさは waterSize で決まる
};

struct WaterPSInput
{
    float4 pos : SV_POSITION;
    float3 worldPos : TEXCOORD0;   // 波で持ち上げた後のワールド座標
};

WaterPSInput VS(WaterVSInput input)
{
    WaterPSInput output;
    // 格子をワールド(オーナーの位置だけの行列)へ置き、その場所の波の高さだけ持ち上げる
    float4 worldPos = mul(float4(input.grid.x * waterSize.x, 0.0f, input.grid.y * waterSize.y, 1.0f), world);
    float h;
    float2 grad;
    EvalWaves(worldPos.xz, h, grad);
    worldPos.y += h;
    output.pos = mul(mul(worldPos, view), proj);
    output.worldPos = worldPos.xyz;
    return output;
}

float4 PS(WaterPSInput input) : SV_TARGET
{
    // 法線: 波の式の傾きから、ピクセルごとに求める(頂点の間でも滑らか)
    float h;
    float2 grad;
    EvalWaves(input.worldPos.xz, h, grad);
    float3 normal = normalize(float3(-grad.x, 1.0f, -grad.y));
    float3 toEye = normalize(eyePos.xyz - input.worldPos);
    if (dot(normal, toEye) < 0.0f) normal = -normal;   // 水の中から見上げた時も、見ている側を向ける(両面描画)

    // 色: 見る角度が浅いほど空の色が映る(フレネル)。空の色は光の影響を受けない
    float fresnel = pow(1.0f - saturate(dot(normal, toEye)), max(waterParams.y, 0.0001f));

    // 平行光源(+環境光)。影を受ける設定の時だけ、平行光源の光を影で暗くする
    float3 toLight = normalize(-lightDir.xyz);
    float ndotl = saturate(dot(normal, toLight));
    float sunShadow = 1.0f;
    if (waterSky.w > 0.5f && shadowParams.x > 0.5f)
    {
        sunShadow = SunShadow(input.worldPos, normal, ndotl);
    }
    float3 lighting = ambient.xyz + lightColor.xyz * (ndotl * sunShadow);

    // 光沢: 平行光源のきらめき(ブリン・フォン)
    float shininess = max(waterParams.z, 1.0f);
    float3 specular = float3(0.0f, 0.0f, 0.0f);
    if (waterParams.x > 0.0f && ndotl > 0.0f)
    {
        float3 halfDir = normalize(toLight + toEye);
        specular += lightColor.xyz * (pow(saturate(dot(normal, halfDir)), shininess) * waterParams.x * sunShadow);
    }

    // 点光源・スポットライト: 光の数が0なら何もしない(ループは回らない)
    if (lightCounts.x > 0)
    {
        [loop]
        for (int i = 0; i < lightCounts.x && i < MAX_LIGHTS; i++)
        {
            GpuLight l = lights[i];
            float3 toLightVec = l.positionRange.xyz - input.worldPos;
            float dist = length(toLightVec);
            float lightRange = l.positionRange.w;
            if (dist >= lightRange) continue;

            // 減衰(届く距離でちょうど0)
            float3 lightDirToPixel = toLightVec / max(dist, 0.0001f);
            float t = saturate(1.0f - (dist / lightRange) * (dist / lightRange));
            float atten = t * t;

            // スポットライト: 向きとの角度で内側は全開、外側で0
            if (l.colorType.w > 1.5f)
            {
                float cosAngle = dot(-lightDirToPixel, l.directionOuter.xyz);
                float s = saturate((cosAngle - l.directionOuter.w) / max(l.innerPad.x - l.directionOuter.w, 0.0001f));
                atten *= s * s * (3.0f - 2.0f * s);
            }

            float lightNdotL = saturate(dot(normal, lightDirToPixel));

            // 影を落とすスポットライト(1つだけ)の影。影を受ける設定の時だけ
            float spotShadowFactor = 1.0f;
            if (l.innerPad.y > 0.5f && waterSky.w > 0.5f && spotParams.x > 0.5f)
            {
                spotShadowFactor = SpotShadow(input.worldPos, normal, lightNdotL);
            }
            float3 radiance = l.colorType.rgb * (atten * spotShadowFactor);
            lighting += radiance * lightNdotL;

            if (waterParams.x > 0.0f && lightNdotL > 0.0f)
            {
                float3 lightHalf = normalize(lightDirToPixel + toEye);
                specular += radiance * (pow(saturate(dot(normal, lightHalf)), shininess) * waterParams.x);
            }
        }
    }

    // 水の色(光を受ける)と空の色をフレネルで混ぜる。αは水の色のαそのまま
    float3 body = waterColor.rgb * lighting;
    float3 color = lerp(body, waterSky.rgb, fresnel);

    // 出力は「αを掛けた色」(ブレンドは ONE / INV_SRC_ALPHA)。きらめきはαで薄まらないよう、αを掛けずに足す
    return float4(color * waterColor.a + specular, waterColor.a);
}
