// 骨で曲げるメッシュ(スキンメッシュ)用のシェーダー。
// 見た目の計算(ライティング・色・テクスチャ)は Shader3D.hlsl と同じで、頂点を骨の行列で動かす所だけが違う。
// b0 は Shader3D.hlsl と同じ構成(ワールド・ビュー・射影・光)、b1 に骨の行列と色を入れる

#define MAX_BONES 64

cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    float4 lightDir;
    float4 lightColor;
    float4 ambient;
};

cbuffer SkinBuffer : register(b1)
{
    matrix bones[MAX_BONES];   // スキン行列(逆バインド行列 × 骨のモデル空間の行列)。モデル空間 = ワールド行列を掛ける前
    float4 tint;               // 全体にかける色(ModelRenderer::SetColor)
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
    output.color = input.color * tint;
    output.uv = input.uv;
    return output;
}

Texture2D tex : register(t0);
SamplerState smp : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    float4 texColor = tex.Sample(smp, input.uv) * input.color;

    // ほぼ透明なピクセルは描画しない(Shader3D.hlslと同じ)
    clip(texColor.a - 0.01f);

    // 平行光源(光が来る方向は-lightDir)+環境光(Shader3D.hlslと同じ)
    float3 normal = normalize(input.worldNormal);
    float ndotl = saturate(dot(normal, normalize(-lightDir.xyz)));
    float3 lighting = ambient.xyz + lightColor.xyz * ndotl;

    float4 finalColor = float4(texColor.rgb * lighting, texColor.a);
    return finalColor;
}
