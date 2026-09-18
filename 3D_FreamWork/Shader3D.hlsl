cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

PSInput VS(VSInput input)
{
    PSInput output;
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, proj);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

Texture2D tex : register(t0);
SamplerState smp : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float4 finalColor = tex.Sample(smp, input.uv) * input.color;

    // ほぼ透明なピクセルは描画しない(色も深度も書き込まない)。
    // これにより、透明な部分だけ裏面(反対側の面)が透けて見えるようになる
    clip(finalColor.a - 0.01f);

    return finalColor;
}
