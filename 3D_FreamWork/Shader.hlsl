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
    float4 finalColor = tex.Sample(smp, input.uv) * input.color;

    // 半透明(アンチエイリアスされた縁など)は描画しない(色も深度も書き込まない)。
    // しきい値を0.01のような小さい値にすると、縁の半透明ピクセルが背景と
    // ブレンドされてしまい、縁が背景色に染まって見える(特にWorldモードで3D背景と
    // 合成される時に目立つ)。0.5でハードカットアウトにすることでこれを防ぐ
    clip(finalColor.a - 0.5f);

    return finalColor;
}