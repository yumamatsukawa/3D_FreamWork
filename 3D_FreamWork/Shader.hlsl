cbuffer ConstantBuffer : register(b0)
{
    matrix world;
};

struct VSInput
{
    float2 pos : POSITION;
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
    output.pos = mul(float4(input.pos, 0.0f, 1.0f), world);
    output.color = input.color;
    output.uv = input.uv;
    return output;
}

Texture2D tex : register(t0);
SamplerState smp : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    return tex.Sample(smp, input.uv) * input.color;
}