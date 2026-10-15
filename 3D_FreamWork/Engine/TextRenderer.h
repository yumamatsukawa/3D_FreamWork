#pragma once
#include "Component.h"
#include "Text.h"
#include <DirectXMath.h>
#include <string>

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
// - 文字の大きさは、カメラからの距離では変わらない(いつもstyle.sizeのピクセル)
// - 文字はいつも画面の一番上に出る(Direct2Dで描くため。画像や3Dのオブジェクトの後ろには隠れない)
class TextRenderer : public Component {
private:
    std::wstring text;                    // 表示する文字
    TextStyle style;                      // 見た目
    DirectX::XMFLOAT3 offset{};           // オブジェクトの位置からずらす量(ワールド)
    bool use3DCamera = false;             // trueなら3D用カメラで画面座標に直す

public:
    void SetText(const std::wstring& value) { text = value; }       // 表示する文字
    const std::wstring& GetText() const { return text; }
    void SetStyle(const TextStyle& value) { style = value; }        // 見た目
    const TextStyle& GetStyle() const { return style; }
    void SetColor(DirectX::XMFLOAT4 color) { style.color = color; } // 文字の色だけ変える(wを下げると縁取りも一緒に薄くなる)
    void SetOffset(DirectX::XMFLOAT3 value) { offset = value; }     // ずらす量(ワールド)
    void SetUse3DCamera(bool value) { use3DCamera = value; }        // 3Dのオブジェクトに付ける時はtrue

    void Draw() override;
};
