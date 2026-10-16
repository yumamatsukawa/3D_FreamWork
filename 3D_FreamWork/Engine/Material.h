#pragma once
#include <DirectXMath.h>
#include <string>

using namespace DirectX;

// 3Dメッシュの見た目の設定をまとめた構造体(色・画像・光沢・自発光など)。
// MeshRenderer / ModelRenderer が1体ごとに持ち、GetMaterial() で手直し、SetMaterial() でまとめて設定できる。
// 例: meshRenderer->GetMaterial().specularStrength = 0.8f;   // つやつやにする
// 既定値のままなら、マテリアルが無かった頃と見た目は同じ(ハイライトなし・自発光なし)。
// ※ライト・影・縁の光・枠線の項目は、それぞれの機能を作る時にここへ足す
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
};

// シェーダー(Shader3DCommon.hlsli の MaterialBuffer, b2)に送る形。16バイトずつ区切って並べる
struct MaterialConstantBuffer {
    XMFLOAT4 color;      // rgba
    XMFLOAT4 specular;   // rgb=色, w=強さ
    XMFLOAT4 emissive;   // rgb=色, w=強さ
    XMFLOAT4 params;     // x=shininess, z=影を受ける(1/0), w=光を使う(1=lit, 0=unlit)。yは予約(縁の光で使う)
};
static_assert(sizeof(MaterialConstantBuffer) % 16 == 0, "定数バッファは16バイト区切り");

// Material をシェーダーに送る形へ変換する
inline MaterialConstantBuffer MakeMaterialConstantBuffer(const Material& material) {
    MaterialConstantBuffer cb;
    cb.color = material.color;
    cb.specular = { material.specularColor.x, material.specularColor.y, material.specularColor.z, material.specularStrength };
    cb.emissive = { material.emissiveColor.x, material.emissiveColor.y, material.emissiveColor.z, material.emissiveStrength };
    cb.params = { material.shininess, 0.f, material.receiveShadow ? 1.f : 0.f, material.lit ? 1.f : 0.f };
    return cb;
}
