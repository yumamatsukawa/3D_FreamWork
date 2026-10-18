#pragma once
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

// 3Dメッシュの見た目の設定をまとめた構造体(色・画像・光沢・自発光など)。
// MeshRenderer / ModelRenderer が1体ごとに持ち、GetMaterial() で手直し、SetMaterial() でまとめて設定できる。
// 例: meshRenderer->GetMaterial().specularStrength = 0.8f;   // つやつやにする
// 既定値のままなら、マテリアルが無かった頃と見た目は同じ(ハイライトなし・自発光なし)。
// 縁の光(rim*)と枠線(outline*)も既定ではオフ(rimStrength = 0、outlineWidth = 0)。
struct Material {
    // ── 基本 ──
    XMFLOAT4     color{ 1.f, 1.f, 1.f, 1.f };   // 色(aが透明度。1=不透明)
    std::wstring texture;                        // 画像のパス(空なら白)。MeshRenderer / ModelRenderer が Resources で読む
    bool         lit = true;                     // false=光の影響を受けない(スカイボックス)
    bool         depthWrite = true;              // false=自分より後ろの物を隠さない(半透明・スカイボックス)

    // ── 光沢(ハイライト) ──
    XMFLOAT3 specularColor{ 1.f, 1.f, 1.f };     // 光沢の色
    float    specularStrength = 0.f;             // 光沢の強さ。0=ハイライト無し(既定)
    float    shininess = 32.f;                   // 光沢の鋭さ(大きいほどハイライトが小さく鋭い)

    // ── 自発光 ──
    XMFLOAT3 emissiveColor{ 1.f, 1.f, 1.f };     // 自分で光る色
    float    emissiveStrength = 0.f;             // 自発光の強さ。0=光らない(既定)

    // ── 影(Shadow を有効にした時だけ意味がある。既定はどちらもOFF=今までと同じ見た目) ──
    bool     castShadow = false;                 // 影を落とすか(MeshRenderer / ModelRenderer が影のパスで描く)
    bool     receiveShadow = false;              // 影を受けるか(平行光源の光だけが影で暗くなる)

    // ── 縁の光(リム発光)。視線に対して斜めの縁(物の輪郭に近い所)を、物の内側で光らせる。strength=0なら光らない(既定) ──
    XMFLOAT3 rimColor{ 1.f, 1.f, 1.f };          // 縁の光の色
    float    rimStrength = 0.f;                  // 縁の光の強さ。0=光らない(既定)
    float    rimPower = 3.f;                     // 縁にどれだけ集まるか(大きいほど縁の細い線だけが光る)

    // ── 枠線(アウトライン)。物の外側に、画面上で outlineWidth ピクセルの太さの線を描く。width=0なら描かない(既定) ──
    XMFLOAT4 outlineColor{ 1.f, 1.f, 0.f, 1.f }; // 枠線の色(aは濃さ。1=不透明)
    float    outlineWidth = 0.f;                 // 枠線の太さ(画面のピクセル。遠くても近くても同じ太さ)。0=枠線なし(既定)
    bool     outlineSeeThrough = false;          // true=手前に壁などがあって隠れていても、枠線を描く(壁越しに見える。既定はfalse)
};

// シェーダー(Shader3DCommon.hlsli の MaterialBuffer, b2)に送る形。16バイトずつ区切って並べる
struct MaterialConstantBuffer {
    XMFLOAT4 color;      // rgba
    XMFLOAT4 specular;   // rgb=色, w=強さ
    XMFLOAT4 emissive;   // rgb=色, w=強さ
    XMFLOAT4 params;     // x=shininess, y=縁の光のpower, z=影を受ける(1/0), w=光を使う(1=lit, 0=unlit)
    XMFLOAT4 rim;        // rgb=縁の光の色, w=強さ(0=光らない)
    XMFLOAT4 outline;    // rgb=枠線の色, w=濃さ
    XMFLOAT4 outlineParams;   // x=枠線の太さ(ピクセル), y=画面の幅, z=画面の高さ(ピクセル。Mesh / SkinMesh が描く時に入れる)
};
static_assert(sizeof(MaterialConstantBuffer) == 16 * 7, "Shader3DCommon.hlsli の MaterialBuffer(float4 が7本)と同じ大きさ");
static_assert(sizeof(MaterialConstantBuffer) % 16 == 0, "定数バッファは16バイト区切り");

// Material をシェーダーに送る形へ変換する
inline MaterialConstantBuffer MakeMaterialConstantBuffer(const Material& material) {
    MaterialConstantBuffer cb;
    cb.color = material.color;
    cb.specular = { material.specularColor.x, material.specularColor.y, material.specularColor.z, material.specularStrength };
    cb.emissive = { material.emissiveColor.x, material.emissiveColor.y, material.emissiveColor.z, material.emissiveStrength };
    cb.params = { material.shininess, material.rimPower, material.receiveShadow ? 1.f : 0.f, material.lit ? 1.f : 0.f };
    cb.rim = { material.rimColor.x, material.rimColor.y, material.rimColor.z, material.rimStrength };
    cb.outline = { material.outlineColor.x, material.outlineColor.y, material.outlineColor.z, material.outlineColor.w };
    cb.outlineParams = { material.outlineWidth, 0.f, 0.f, 0.f };
    return cb;
}
