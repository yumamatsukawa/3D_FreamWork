#include "FadeTransition.h"
#include "../Render/Graphics.h"

FadeTransition::FadeTransition(DirectX::XMFLOAT4 fadeColor, float fadeDuration)
    : color{ fadeColor.x, fadeColor.y, fadeColor.z }, duration(fadeDuration) {
}

FadeTransition::FadeTransition(const FadeTransition& other)
    : SceneTransition(other), color(other.color), duration(other.duration) {
}

FadeTransition::~FadeTransition() {
    ReleaseBrush();
}

void FadeTransition::DrawCover(ID2D1RenderTarget* target, float progress) {
    Fill(target, progress);         // だんだん濃くなる
}

void FadeTransition::DrawOpen(ID2D1RenderTarget* target, float progress) {
    Fill(target, 1.f - progress);   // だんだん薄くなる
}

void FadeTransition::Fill(ID2D1RenderTarget* target, float alpha) {
    if (alpha <= 0.f) return;

    // 描画先が変わっていたら、前の描画先で作ったブラシは使えないので作り直す
    if (target != brushTarget) {
        ReleaseBrush();
        brushTarget = target;
    }
    if (!brush) target->CreateSolidColorBrush(D2D1::ColorF(0.f, 0.f, 0.f, 0.f), &brush);
    if (!brush) return;

    brush->SetColor(D2D1::ColorF(color.x, color.y, color.z, alpha));
    target->FillRectangle(D2D1::RectF(0.f, 0.f, Graphics::width, Graphics::height), brush);
}

void FadeTransition::ReleaseBrush() {
    if (brush) { brush->Release(); brush = nullptr; }
    brushTarget = nullptr;
}
