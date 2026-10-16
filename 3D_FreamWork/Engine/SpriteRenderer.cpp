#include "SpriteRenderer.h"
#include "GameObject.h"
#include "Resources.h"
#include "Visibility.h"
#include <cmath>

SpriteRenderer::SpriteRenderer(const std::wstring& filepath, int cols, int rows) {
    SetTexture(filepath, cols, rows);
}

void SpriteRenderer::SetTexture(const std::wstring& filepath, int cols, int rows) {
    // 新しい画像を先に借りてから、前の画像を返す(同じ画像を設定し直しても読み直さないように)
    unsigned int newTexID = Resources::LoadTexture(filepath);
    if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
    texID = newTexID;

    // 分割数が変わるのでコマは最初に戻す(0以下だと0で割ってしまうので1にする)
    sheet = { cols > 0 ? cols : 1, rows > 0 ? rows : 1, 0 };
}

void SpriteRenderer::SetSpriteIndex(int index) {
    sheet.index = index;
}

void SpriteRenderer::SetColor(float r, float g, float b, float a) {
    color = { r, g, b, a };
}

DirectX::XMFLOAT4 SpriteRenderer::GetColor() const {
    return color;
}

void SpriteRenderer::Draw() {
    // 親をたどったワールド空間のTransformで描画する(親と一緒に動く/回る/拡縮する)
    Transform world = GetOwner()->transform.GetWorldTransform();

    // 画面の外なら描かない(板の対角線の半分を半径にした球・円で調べる。回転しても入る大きさ)
    if (frustumCulling && world.scale.x > 0.f && world.scale.y > 0.f) {
        float radius = 0.5f * sqrtf(world.scale.x * world.scale.x + world.scale.y * world.scale.y);
        bool visible = worldSpace ? Visibility::IsSphereInView3D(world.position, radius)
                                  : Visibility::IsCircleInView2D(world.position, radius);
        if (!visible) {
            Visibility::CountCulled();
            return;
        }
    }

    Image::Draw(world, texID, color, sheet, worldSpace, billboardLockX, billboardLockY, billboardLockZ);
}

void SpriteRenderer::Uninit() {
    // 借りた画像を返す(他に使っている人がいるかもしれないので、ここでは解放しない)
    if (texID != UINT_MAX) Resources::ReleaseTexture(texID);
    texID = UINT_MAX;
}
