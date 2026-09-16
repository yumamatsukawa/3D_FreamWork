#pragma once
#include "Component.h"
#include "Image.h"
#include <string>

// GameObjectに「テクスチャを描画する見た目」を持たせるComponent。
// UnityのSpriteRendererに相当する。
class SpriteRenderer : public Component {
private:
    unsigned int texID = UINT_MAX;

public:
    SpriteRenderer() = default;
    // 生成と同時にテクスチャを指定したい場合はこちら
    // 例: obj->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
    SpriteRenderer(const std::wstring& filepath, int cols = 1, int rows = 1);

    void SetTexture(const std::wstring& filepath, int cols = 1, int rows = 1);
    void SetSpriteIndex(int index);
    void SetColor(float r, float g, float b, float a);
    DirectX::XMFLOAT4 GetColor() const;

    void Draw() override;
    void Uninit() override;
};
