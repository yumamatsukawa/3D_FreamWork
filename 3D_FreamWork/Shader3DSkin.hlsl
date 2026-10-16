// 骨で曲げるメッシュ(スキンメッシュ)用のシェーダー。
// 見た目の計算(ライティング・色・テクスチャ・光沢・自発光)は Shader3D.hlsl と同じ Shader3DCommon.hlsli で、頂点を骨の行列で動かす所だけが違う。
// b0 は Shader3D.hlsl と同じ構成(ワールド・ビュー・射影・光)、b1 に骨の行列を入れる(色は b2 のマテリアル)
#include "Shader3DCommon.hlsli"

#define MAX_BONES 64

cbuffer SkinBuffer : register(b1)
{
    matrix bones[MAX_BONES];   // スキン行列(逆バインド行列 × 骨のモデル空間の行列)。モデル空間 = ワールド行列を掛ける前
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    uint4 boneIndices : BLENDINDICES;
    float4 boneWeights : BLENDWEIGHT;
};

PSInput VS(VSInput input)
{
    PSInput output;

    // 影響する最大4本の骨の行列を、影響度の割合で混ぜる(リニアブレンドスキニング)
    matrix skin = bones[input.boneIndices.x] * input.boneWeights.x
                + bones[input.boneIndices.y] * input.boneWeights.y
                + bones[input.boneIndices.z] * input.boneWeights.z
                + bones[input.boneIndices.w] * input.boneWeights.w;

    float4 skinnedPos = mul(float4(input.pos, 1.0f), skin);
    float3 skinnedNormal = mul(float4(input.normal, 0.0f), skin).xyz;

    float4 worldPos = mul(skinnedPos, world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, proj);
    output.worldNormal = normalize(mul(float4(skinnedNormal, 0.0f), world).xyz);
    output.color = input.color;
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    return output;
}

float4 PS(PSInput input) : SV_TARGET
{
    return ShadePixel(input);
}
