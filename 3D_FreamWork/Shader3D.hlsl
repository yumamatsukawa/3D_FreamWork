// 普通の3Dメッシュ用のシェーダー。見た目の計算(光・色・テクスチャ・光沢・自発光・縁の光)は Shader3DCommon.hlsli にある
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

// 影の深度パス用(ピクセルシェーダー無し)。b0 の view / proj には、光から見た行列が入っている
float4 VSShadow(VSInput input) : SV_POSITION
{
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    float4 viewPos = mul(worldPos, view);
    return mul(viewPos, proj);
}

// 枠線用(Mesh::DrawOutline)。slot 1 の NORMAL1 は、同じ位置の法線を平均した法線(角で枠線が割れないように)
struct OutlineVSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL0;
    float3 smoothNormal : NORMAL1;
};

PSInput VSOutline(OutlineVSInput input)
{
    PSInput output;
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    float4 clipPos = mul(mul(worldPos, view), proj);
    float3 smoothWorldNormal = normalize(mul(float4(input.smoothNormal, 0.0f), world).xyz);
    output.pos = OutlineClipPos(clipPos, smoothWorldNormal);
    output.worldNormal = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
    output.uv = float2(0.0f, 0.0f);
    output.worldPos = worldPos.xyz;
    return output;
}

float4 PSOutline(PSInput input) : SV_TARGET
{
    return ShadeOutline(input);
}

float4 PS(PSInput input) : SV_TARGET
{
    return ShadePixel(input);
}
