#include "TextRenderer.h"
#include "../Core/GameObject.h"
#include "../Render/Image.h"

using namespace DirectX;

void TextRenderer::Draw() {
    if (text.empty()) return;
    const Transform& transform = GetOwner()->transform;

    // 3D: 3D用カメラで画面座標に直す(カメラの後ろなら描かない)
    if (use3DCamera) {
        XMFLOAT3 world;
        XMStoreFloat3(&world, XMVector3TransformCoord(XMVectorZero(), transform.GetWorldMatrix()));
        XMFLOAT3 screen = Image::WorldToScreen3D({ world.x + offset.x, world.y + offset.y, world.z + offset.z });
        if (screen.z <= 0.f) return;
        Text::Draw(text, screen.x, screen.y, style);
        return;
    }

    // 2D: 2D用カメラで画面座標に直す(SpriteRendererと同じく、親と合わせたワールドの位置を使う)
    XMFLOAT3 world = transform.GetWorldTransform().position;
    XMFLOAT2 screen = Image::WorldToScreen2D({ world.x + offset.x, world.y + offset.y, world.z + offset.z });
    Text::Draw(text, screen.x, screen.y, style);
}
