// 普通の3Dメッシュ用のシェーダー。見た目の計算(光・色・テクスチャ・光沢・自発光)は Shader3DCommon.hlsli にある
#include "Shader3DCommon.hlsli"

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

PSInput VS(VSInput input)
{
    PSInput output;
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, proj);
    output.worldNormal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.color = input.color;
    output.uv = input.uv;
    output.worldPos = worldPos.xyz;
    return output;
}

float4 PS(PSInput input) : SV_TARGET
{
    return ShadePixel(input);
}
