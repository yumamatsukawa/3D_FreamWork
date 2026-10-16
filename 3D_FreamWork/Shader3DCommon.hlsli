// Shader3D.hlsl と Shader3DSkin.hlsl の共通部分(定数バッファ・ピクセルシェーダーの見た目の計算)。
// 光や見た目の計算を変える時は、ここだけを直す(2つのシェーダーが同じ結果になる)。
// レジスタ: b0=ワールド・ビュー・射影・平行光源, b1=骨の行列(Skinだけ), b2=マテリアル, b3=カメラ位置, b4=点光源・スポットライト, t0/s0=画像

cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    float4 lightDir;
    float4 lightColor;
    float4 ambient;
};

// マテリアル(Material.h の MaterialConstantBuffer と同じ並び)
cbuffer MaterialBuffer : register(b2)
{
    float4 matColor;      // 色(頂点の色に掛ける)
    float4 matSpecular;   // rgb=光沢の色, w=強さ(0=ハイライト無し)
    float4 matEmissive;   // rgb=自発光の色, w=強さ(0=光らない)
    float4 matParams;     // x=shininess, w=光を使う(1=lit, 0=unlit)
};

// カメラ(Mesh.h の MeshCameraBuffer と同じ並び)
cbuffer CameraBuffer : register(b3)
{
    float4 eyePos;        // xyz=カメラのワールド座標
};

// 点光源・スポットライト(Lighting.h の LightConstantBuffer と同じ並び)
#define MAX_LIGHTS 8
struct GpuLight
{
    float4 positionRange;    // xyz=位置, w=届く距離
    float4 colorType;        // rgb=色×強さ, w=種類(0=無し, 1=点, 2=スポット)
    float4 directionOuter;   // xyz=向き(スポットの中心へ進む向き), w=cos(外側の角度)
    float4 innerPad;         // x=cos(内側の角度)。残りは予約(影で使う)
};
cbuffer LightBuffer : register(b4)
{
    int4 lightCounts;        // x=光の数(0なら今までと同じ見た目)
    GpuLight lights[MAX_LIGHTS];
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 worldNormal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 worldPos : TEXCOORD1;   // ワールド座標(光沢の計算でカメラへの向きを求めるのに使う)
};

Texture2D tex : register(t0);
SamplerState smp : register(s0);

// ピクセルの色を決める(Shader3D.hlsl と Shader3DSkin.hlsl のPSから呼ぶ)
float4 ShadePixel(PSInput input)
{
    float4 texColor = tex.Sample(smp, input.uv) * input.color * matColor;

    // ほぼ透明なピクセルは描画しない(色も深度も書き込まない)。
    // これにより、透明な部分だけ裏面(反対側の面)が透けて見えるようになる
    clip(texColor.a - 0.01f);

    // 平行光源(光が来る方向は-lightDir)+環境光でシンプルな陰影をつける
    float3 normal = normalize(input.worldNormal);
    float3 toLight = normalize(-lightDir.xyz);
    float ndotl = saturate(dot(normal, toLight));
    float3 lighting = ambient.xyz + lightColor.xyz * ndotl;

    // 光の影響を受けない(unlit)時は、いつも同じ明るさ
    bool useLight = matParams.w > 0.5f;
    if (!useLight) lighting = float3(1.0f, 1.0f, 1.0f);

    // 点光源・スポットライト: 光の数が0なら何もしない(今までと同じ見た目)。unlit の物は光の影響を受けない
    float3 extraSpecular = float3(0.0f, 0.0f, 0.0f);
    if (useLight && lightCounts.x > 0)
    {
        float3 toEye = normalize(eyePos.xyz - input.worldPos);
        float shininess = max(matParams.x, 1.0f);
        float3 extraDiffuse = float3(0.0f, 0.0f, 0.0f);

        [loop]
        for (int i = 0; i < lightCounts.x && i < MAX_LIGHTS; i++)
        {
            GpuLight l = lights[i];
            float3 toLightVec = l.positionRange.xyz - input.worldPos;
            float dist = length(toLightVec);
            float lightRange = l.positionRange.w;
            if (dist >= lightRange) continue;   // 届かない距離

            // 減衰: 距離が届く距離(range)でちょうど0になる
            float3 lightDirToPixel = toLightVec / max(dist, 0.0001f);
            float t = saturate(1.0f - (dist / lightRange) * (dist / lightRange));
            float atten = t * t;

            // スポットライト: 向きとの角度が、内側なら全開、外側で0、間は滑らかにつなぐ
            if (l.colorType.w > 1.5f)
            {
                float cosAngle = dot(-lightDirToPixel, l.directionOuter.xyz);
                float s = saturate((cosAngle - l.directionOuter.w) / max(l.innerPad.x - l.directionOuter.w, 0.0001f));
                atten *= s * s * (3.0f - 2.0f * s);
            }

            float3 radiance = l.colorType.rgb * atten;
            float lightNdotL = saturate(dot(normal, lightDirToPixel));
            extraDiffuse += radiance * lightNdotL;

            // 光沢も同じ光で光らせる
            if (matSpecular.w > 0.0f && lightNdotL > 0.0f)
            {
                float3 lightHalf = normalize(lightDirToPixel + toEye);
                float lightSpec = pow(saturate(dot(normal, lightHalf)), shininess) * matSpecular.w;
                extraSpecular += matSpecular.rgb * radiance * lightSpec;
            }
        }
        lighting += extraDiffuse;
    }

    float3 rgb = texColor.rgb * lighting;

    // 光沢(ブリン・フォン): 光とカメラの真ん中の向きと法線が近いほど光る
    if (useLight && matSpecular.w > 0.0f && ndotl > 0.0f)
    {
        float3 toEye = normalize(eyePos.xyz - input.worldPos);
        float3 halfDir = normalize(toLight + toEye);
        float spec = pow(saturate(dot(normal, halfDir)), max(matParams.x, 1.0f)) * matSpecular.w;
        rgb += matSpecular.rgb * lightColor.xyz * spec;
    }
    rgb += extraSpecular;

    // 自発光: 光の計算の後に足す(暗い所でもその色で見える)
    if (matEmissive.w > 0.0f)
    {
        rgb += matEmissive.rgb * matEmissive.w;
    }

    return float4(rgb, texColor.a);
}
