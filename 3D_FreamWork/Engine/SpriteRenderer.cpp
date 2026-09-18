#include "SpriteRenderer.h"
#include "GameObject.h"

SpriteRenderer::SpriteRenderer(const std::wstring& filepath, int cols, int rows) {
    SetTexture(filepath, cols, rows);
}

void SpriteRenderer::SetTexture(const std::wstring& filepath, int cols, int rows) {
    texID = Image::LoadTexture(filepath, cols, rows);
}

void SpriteRenderer::SetSpriteIndex(int index) {
    Image::SetSpriteIndex(texID, index);
}

void SpriteRenderer::SetColor(float r, float g, float b, float a) {
    Image::SetColor(texID, r, g, b, a);
}

DirectX::XMFLOAT4 SpriteRenderer::GetColor() const {
    return Image::GetColor(texID);
}

void SpriteRenderer::Draw() {
    // 親をたどったワールド空間のTransformで描画する(親と一緒に動く/回る/拡縮する)
    Image::Draw(GetOwner()->transform.GetWorldTransform(), texID, worldSpace);
}

void SpriteRenderer::Uninit() {
    Image::ReleaseTexture(texID);
}
