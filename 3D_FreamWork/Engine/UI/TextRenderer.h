#pragma once
#include "../Core/Component.h"
#include "Text.h"
#include <DirectXMath.h>
#include <string>

// 文字の大きさの決め方
enum class TextSizeMode {
    Screen,   // カメラからの距離では変わらない(style.sizeのピクセル)
    World     // 3Dカメラからの奥行きに反比例して変わる(3D用カメラの時だけ)
};

// GameObjectに付いて一緒に動く文字(ダメージの数字・キャラクターの名前など)を描くComponent。
//
// 使い方:
//   auto* text = obj->AddComponent<TextRenderer>();
//   text->SetText(L"+10");
//   text->SetStyle(style);                  // 大きさ・色・フォントなど(TextStyle)
//   text->SetOffset({ 0.f, 60.f, 0.f });    // オブジェクトの位置から、ワールドでずらす量(頭の上など)
//
// 決まり:
// - 2D用カメラ(Image::GetCamera())で、オブジェクトの位置(親がいれば親と合わせたワールドの位置)+offset を
//   画面座標に直して描く(Image::WorldToScreen2D。SpriteRendererのUIモードと同じく、カメラと一緒にスクロールする)
// - 3Dのオブジェクトに付ける時は SetUse3DCamera(true)。3D用カメラ(Image::GetCamera3D())で画面座標に直し
//   (Image::WorldToScreen3D)、カメラの後ろにある時は描かない
// - 文字の大きさは SetSizeMode で選ぶ(既定は Screen)
//     Screen: カメラからの距離では変わらない(いつもstyle.sizeのピクセル)
//     World : 3Dカメラからの奥行きに反比例して変わる(遠いほど小さく、近いほど大きい。3Dの物と同じ遠近感)。
//             referenceDistance だけ奥にある時が style.size のピクセル。小さくなりすぎ/大きくなりすぎは SetSizeRange で止まる。
//             縁取りの太さも、大きさに合わせて変わる
//     ※ World が効くのは SetUse3DCamera(true) の時だけ。2D用カメラには奥行きが無いので、World を選んでも大きさは変わらない
// - 文字はいつも画面の一番上に出る(Direct2Dで描くため。画像や3Dのオブジェクトの後ろには隠れない)
class TextRenderer : public Component {
private:
    static constexpr float defaultReferenceDistance = 500.f;   // 基準の奥行きの既定(Camera::focalLength の既定と同じ。この奥行きの3Dの物が、2Dと同じ大きさで見える)
    static constexpr float defaultMinSize = 8.f;               // 小さくなる限界の既定(ピクセル)
    static constexpr float defaultMaxScale = 3.f;              // 大きくなる限界の既定(style.sizeの何倍か)

    std::wstring text;                    // 表示する文字
    TextStyle style;                      // 見た目
    DirectX::XMFLOAT3 offset{};           // オブジェクトの位置からずらす量(ワールド)
    bool use3DCamera = false;             // trueなら3D用カメラで画面座標に直す
    TextSizeMode sizeMode = TextSizeMode::Screen;   // 大きさの決め方
    float referenceDistance = defaultReferenceDistance;   // World: style.size のピクセルになる奥行き(ワールド)
    float minSize = defaultMinSize;       // World: 小さくなる限界(ピクセル)
    float maxSize = 0.f;                  // World: 大きくなる限界(ピクセル)。0以下ならstyle.sizeの3倍

public:
    void SetText(const std::wstring& value) { text = value; }       // 表示する文字
    const std::wstring& GetText() const { return text; }
    void SetStyle(const TextStyle& value) { style = value; }        // 見た目
    const TextStyle& GetStyle() const { return style; }
    void SetColor(DirectX::XMFLOAT4 color) { style.color = color; } // 文字の色だけ変える(wを下げると縁取りも一緒に薄くなる)
    void SetOffset(DirectX::XMFLOAT3 value) { offset = value; }     // ずらす量(ワールド)
    void SetUse3DCamera(bool value) { use3DCamera = value; }        // 3Dのオブジェクトに付ける時はtrue
    void SetSizeMode(TextSizeMode value) { sizeMode = value; }      // 大きさの決め方(Screen: 変わらない / World: 奥行きで変わる。3Dカメラの時だけ)
    TextSizeMode GetSizeMode() const { return sizeMode; }
    void SetReferenceDistance(float value) { referenceDistance = value; }   // World: style.size のピクセルになる奥行き(0以下は無視して既定の500)
    float GetReferenceDistance() const { return referenceDistance; }
    void SetSizeRange(float minPixels, float maxPixels) { minSize = minPixels; maxSize = maxPixels; }   // World: 大きさの限界(ピクセル)。maxPixelsが0以下ならstyle.sizeの3倍

    void Draw() override;
};
