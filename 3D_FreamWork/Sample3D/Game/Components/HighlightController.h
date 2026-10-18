#pragma once
#include "../../../Engine/Core/Component.h"
#include <DirectXMath.h>

class ModelRenderer;

// 「Playerが近づいたら、モデルの枠線が光る」動きを担当する(取れる物・調べられる物の合図に使う)。
// 枠線の描き方は ModelRenderer(骨で曲げるモデルにも付く)が担当し、ここでは太さを切り替えるだけ
class HighlightController : public Component {
private:
    ModelRenderer* renderer = nullptr;                    // 枠線を付けるモデル(同じオブジェクトに付いている)
    DirectX::XMFLOAT4 color{ 1.0f, 0.9f, 0.2f, 1.0f };    // 枠線の色
    float width = 3.0f;                                   // 光っている時の枠線の太さ(画面のピクセル)
    float radius = 600.0f;                                // この距離までPlayerが近づいたら光る

public:
    // 光る色・太さ・Playerが近づく距離を、CreateModelDisplay() などから渡してもらう
    void Setup(DirectX::XMFLOAT4 outlineColor, float outlineWidth, float nearRadius) {
        color = outlineColor;
        width = outlineWidth;
        radius = nearRadius;
    }

    void Init() override;
    void Update(float dt) override;
};