cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    float4 lightDir;
    float4 lightColor;
    float4 ambient;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 worldNormal : NORMAL;
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
    return output;
}

Texture2D tex : register(t0);
SamplerState smp : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float4 texColor = tex.Sample(smp, input.uv) * input.color;

    // ほぼ透明なピクセルは描画しない(色も深度も書き込まない)。
    // これにより、透明な部分だけ裏面(反対側の面)が透けて見えるようになる
    clip(texColor.a - 0.01f);

    // 平行光源(光が来る方向は-lightDir)+環境光でシンプルな陰影をつける
    float3 normal = normalize(input.worldNormal);
    float ndotl = saturate(dot(normal, normalize(-lightDir.xyz)));
    float3 lighting = ambient.xyz + lightColor.xyz * ndotl;

    float4 finalColor = float4(texColor.rgb * lighting, texColor.a);
    return finalColor;
}
