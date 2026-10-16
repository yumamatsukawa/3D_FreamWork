// Shader3D.hlsl と Shader3DSkin.hlsl の共通部分(定数バッファ・ピクセルシェーダーの見た目の計算)。
// 光や見た目の計算を変える時は、ここだけを直す(2つのシェーダーが同じ結果になる)。
// レジスタ(MaterialBuffer は 縁の光・枠線の項目も持つ): b0=ワールド・ビュー・射影・平行光源, b1=骨の行列(Skinだけ), b2=マテリアル, b3=カメラ位置, b4=点光源・スポットライト,
//           b5=影, t0/s0=画像, t1=平行光源の影, t2=スポットライトの影, s1=影の比較サンプラー

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
    float4 matParams;     // x=shininess, y=縁の光のpower, z=影を受ける(1/0), w=光を使う(1=lit, 0=unlit)
    float4 matRim;        // rgb=縁の光の色, w=強さ(0=光らない)
    float4 matOutline;    // rgb=枠線の色, w=濃さ
    float4 matOutlineParams;   // x=枠線の太さ(ピクセル), y=画面の幅, z=画面の高さ(ピクセル)
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

// 影(Shadow.h の ShadowConstantBuffer と同じ並び)。影を使わないフレームは何もセットされず、全部0として読む(=影なし)
cbuffer ShadowBuffer : register(b5)
{
    matrix shadowViewProj;   // 平行光源から見た view × proj
    float4 shadowParams;     // x=影を使う(1=ON), y=depthBias, z=normalBias(ワールド単位), w=strength
    float4 shadowParams2;    // x=1/解像度, y=ぼかしの半径(pcfRadius)
    matrix spotViewProj;     // 影を落とすスポットライトから見た view × proj(透視投影)
    float4 spotParams;       // x=スポットの影を使う(1=ON), y=depthBias(ワールド単位), z=near, w=far
    float4 spotParams2;      // x=1/解像度, y=ぼかしの半径(pcfRadius), z=normalBias(ワールド単位), w=strength
};
Texture2D shadowMap : register(t1);       // 平行光源の影の深度
Texture2D spotShadowMap : register(t2);   // スポットライトの影の深度
SamplerComparisonState shadowSmp : register(s1);

// 影のテクスチャの uv の位置を、周りのテクセルも含めて比べて、明るい割合(0〜1)を返す(PCF)。
// 比較サンプラーは、1回で2x2の補間までしてくれる
float ShadowVisibility(Texture2D map, float2 uv, float refDepth, float texel, int radius)
{
    float lit = 0.0f;
    [loop]
    for (int y = -radius; y <= radius; y++)
    {
        [loop]
        for (int x = -radius; x <= radius; x++)
        {
            lit += map.SampleCmpLevelZero(shadowSmp, uv + float2(x, y) * texel, refDepth);
        }
    }
    return lit / (float)((radius * 2 + 1) * (radius * 2 + 1));
}

// 範囲の縁に近づくほど影を弱くする(範囲の端で、影が急に消えて見えないように)。1=影そのまま、0=影なし
float ShadowEdgeFade(float2 uv)
{
    float edge = min(min(uv.x, 1.0f - uv.x), min(uv.y, 1.0f - uv.y));
    return saturate(edge / 0.04f);
}

// 平行光源の影: 0〜1を返す(1=影なし、1-strength=一番濃い影)。ndotl は法線と光の向きの内積(0〜1)
float SunShadow(float3 worldPos, float3 normal, float ndotl)
{
    // 光に対して斜めの面ほど、法線の方向へ多くずらして判定する(自分の面に縞模様の影が出るのを防ぐ)
    float3 shadowPos = worldPos + normal * (shadowParams.z * (1.0f - ndotl));
    float4 clip = mul(float4(shadowPos, 1.0f), shadowViewProj);
    float3 ndc = clip.xyz / clip.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);

    // 影の範囲の外は影なし
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f) return 1.0f;

    float visibility = ShadowVisibility(shadowMap, uv, ndc.z - shadowParams.y, shadowParams2.x, (int)shadowParams2.y);
    visibility = lerp(1.0f, visibility, ShadowEdgeFade(uv));
    return 1.0f - shadowParams.w * (1.0f - visibility);
}

// スポットライトの影: 0〜1を返す(1=影なし)。lightNdotL は法線と光への向きの内積(0〜1)
float SpotShadow(float3 worldPos, float3 normal, float lightNdotL)
{
    float3 shadowPos = worldPos + normal * (spotParams2.z * (1.0f - lightNdotL));
    float4 clip = mul(float4(shadowPos, 1.0f), spotViewProj);
    if (clip.w <= 0.0f) return 1.0f;   // 光の後ろ側
    float3 ndc = clip.xyz / clip.w;
    float2 uv = float2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f) return 1.0f;

    // 透視投影の深度は、光から遠いほど細かい(同じ深度の差でも、遠いほど長いワールドの距離になる)。
    // ワールド単位の depthBias を、その場所の深度の傾き(dz/dd = far×near / (d²×(far-near)))で、深度の差に直して引く
    float nearZ = spotParams.z;
    float farZ = spotParams.w;
    float biasInDepth = spotParams.y * (farZ * nearZ) / (clip.w * clip.w * (farZ - nearZ));
    float visibility = ShadowVisibility(spotShadowMap, uv, ndc.z - biasInDepth, spotParams2.x, (int)spotParams2.y);
    visibility = lerp(1.0f, visibility, ShadowEdgeFade(uv));
    return 1.0f - spotParams2.w * (1.0f - visibility);
}
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
    bool useLight = matParams.w > 0.5f;

    // 影: 影を使っていて、この物が影を受ける時だけ求める。平行光源の光だけを暗くする(環境光・点光源・スポットは暗くしない)
    float sunShadow = 1.0f;
    if (useLight && matParams.z > 0.5f && shadowParams.x > 0.5f)
    {
        sunShadow = SunShadow(input.worldPos, normal, ndotl);
    }
    float3 lighting = ambient.xyz + lightColor.xyz * (ndotl * sunShadow);

    // 光の影響を受けない(unlit)時は、いつも同じ明るさ
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

            float lightNdotL = saturate(dot(normal, lightDirToPixel));

            // 影を落とすスポットライト(1つだけ)は、影を受ける物にだけ影を引く
            float spotShadowFactor = 1.0f;
            if (l.innerPad.y > 0.5f && matParams.z > 0.5f && spotParams.x > 0.5f)
            {
                spotShadowFactor = SpotShadow(input.worldPos, normal, lightNdotL);
            }
            float3 radiance = l.colorType.rgb * (atten * spotShadowFactor);
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
        rgb += matSpecular.rgb * lightColor.xyz * (spec * sunShadow);
    }
    rgb += extraSpecular;

    // 自発光: 光の計算の後に足す(暗い所でもその色で見える)
    if (matEmissive.w > 0.0f)
    {
        rgb += matEmissive.rgb * matEmissive.w;
    }

    // 縁の光(リム発光): 視線に対して斜めの面(輪郭に近い所)ほど光る。裏側を向いた面は、表と同じに扱う(abs)
    if (matRim.w > 0.0f)
    {
        float3 rimToEye = normalize(eyePos.xyz - input.worldPos);
        float rimAmount = pow(1.0f - saturate(abs(dot(normal, rimToEye))), max(matParams.y, 0.01f)) * matRim.w;
        rgb += matRim.rgb * rimAmount;
    }

    return float4(rgb, texColor.a);
}

// ───── 枠線(アウトライン) ─────
// 同じ物をもう一度、法線の向きに画面上で matOutlineParams.x ピクセルだけ膨らませて描き、裏向きの面だけを単色で残す。
// 本物は先に描いてあるので、膨らんだ分(物の外側の輪)だけが枠線として見える。
// 頂点シェーダーが呼ぶ: clipPos=普通にクリップ空間へ変換した位置、smoothNormal=同じ位置の法線を平均したワールド法線
float4 OutlineClipPos(float4 clipPos, float3 smoothNormal)
{
    if (clipPos.w < 0.001f) return clipPos;   // カメラの後ろ側は動かさない

    // 法線の向きを、画面の上でどちらへ動くか(ピクセル単位の向き)に直す: d(clip.xy / clip.w) / dt
    float4 normalClip = mul(mul(float4(smoothNormal, 0.0f), view), proj);
    float2 ndc = clipPos.xy / clipPos.w;
    float2 ndcDirection = (normalClip.xy - ndc * normalClip.w) / clipPos.w;
    float2 pixelDirection = ndcDirection * matOutlineParams.yz * 0.5f;
    float pixelLength = length(pixelDirection);
    if (pixelLength < 0.000001f) return clipPos;   // 法線がカメラの方を向いている(向きが決まらない)

    // 太さ(ピクセル)を NDC の長さに直して、クリップ空間では w を掛ける(遠くても近くても画面上で同じ太さ)
    float2 offsetNdc = (pixelDirection / pixelLength) * matOutlineParams.x * 2.0f / matOutlineParams.yz;
    float4 result = clipPos;
    result.xy += offsetNdc * clipPos.w;

    // 少しだけ手前(カメラ側)へずらす。地面に立つ物の接地部では、膨らんだ殻が地面と同じ高さか地面の下に入るので、
    // 奥のままだと地面に隠されて枠線が切れる。ずらす量は「太さ(ピクセル)× 3 を、その位置のワールドの長さに直した値」(遠くても近くても画面上で同じ割合)。
    // 本物の内側へ枠線が漏れないよう、C++側(DrawOutline)が、本物の見えている画素にはステンシルで枠線を描かない。手前の壁などは、これより大きく手前にあれば枠線を隠す
    float pixelWorld = 2.0f * clipPos.w / (mul(float4(0.0f, 1.0f, 0.0f, 0.0f), proj).y * matOutlineParams.z);
    float shift = matOutlineParams.x * pixelWorld * 3.0f;
    // 奥行き(view空間の距離 = clip.w)を shift だけ手前にした時の深度に直す(xy は動かさない)
    float projA = mul(float4(0.0f, 0.0f, 1.0f, 0.0f), proj).z;
    float projB = mul(float4(0.0f, 0.0f, 0.0f, 1.0f), proj).z;
    float nearerDepth = max(clipPos.w - shift, 0.001f);
    result.z = ((projA * nearerDepth + projB) / nearerDepth) * clipPos.w;
    return result;
}

// 枠線のピクセルシェーダー: 元の面が手前(カメラ側)を向いているピクセルは捨てる(裏向きの面だけを残す)
float4 ShadeOutline(PSInput input)
{
    float3 toEye = eyePos.xyz - input.worldPos;
    if (dot(input.worldNormal, toEye) > 0.0f) discard;
    return float4(matOutline.rgb, matOutline.w);
}
