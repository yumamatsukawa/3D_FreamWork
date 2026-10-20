#pragma once
#include "../../../../Engine/Core/Component.h"
#include <DirectXMath.h>

// Rectデモの操作役。画面の9か所(Anchor)に印と四角を出し、ドラッグで動かせる3つの四角(Image::DrawRect)を見せる。
// 画面サイズが変わっても、Anchorに置いた物は隅や中央からの距離を保つ
class RectDemoController : public Component {
private:
    // ドラッグで動かせる四角
    struct DragRect {
        DirectX::XMFLOAT2 center;     // 中心の画面座標(画面中央が(0,0)、Y+=上)
        DirectX::XMFLOAT2 size;       // 幅と高さ
        DirectX::XMFLOAT4 color;      // 色
    };

    static constexpr int dragCount = 3;       // ドラッグできる四角の数

    DragRect rects[dragCount];                // ドラッグできる四角(後ろの物ほど手前に描く)
    int dragging = -1;                        // 今つかんでいる四角の番号(つかんでいない時は-1)
    DirectX::XMFLOAT2 grabOffset{ 0.f, 0.f }; // つかんだ時の、マウスから四角の中心までのずれ

    int FindRectAt(const DirectX::XMFLOAT2& point) const;   // 点の下にある一番手前の四角の番号(無ければ-1)

public:
    RectDemoController();

    void Update(float dt) override;
    void Draw() override;
};
