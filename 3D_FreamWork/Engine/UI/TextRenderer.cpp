#include "TextRenderer.h"
#include "../Core/GameObject.h"
#include "../Render/Image.h"
#include <algorithm>
#include <cmath>

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

        // Screen: 大きさはそのまま
        if (sizeMode == TextSizeMode::Screen || !(style.size > 0.f)) {
            Text::Draw(text, screen.x, screen.y, style);
            return;
        }

        // World: 奥行き(screen.z)に反比例して、基準の奥行きでstyle.sizeになる大きさにする(styleのコピーを書き換える)
        const float reference = referenceDistance > 0.f ? referenceDistance : defaultReferenceDistance;
        const float upper = maxSize > 0.f ? maxSize : style.size * defaultMaxScale;
        float size = style.size * reference / screen.z;
        size = (std::min)((std::max)(size, minSize), (std::max)(upper, minSize));   // 小さすぎる/大きすぎる時は限界で止める(描かない事はしない)
        size = (std::max)(std::round(size), 1.f);   // 整数のピクセルにして、フォント設定の取っておきを使い回す(毎フレーム作り直さない)
        TextStyle scaled = style;
        scaled.size = size;
        scaled.outlineWidth = style.outlineWidth * size / style.size;   // 縁取りも大きさに比例させる
        Text::Draw(text, screen.x, screen.y, scaled);
        return;
    }

    // 2D: 2D用カメラで画面座標に直す(SpriteRendererと同じく、親と合わせたワールドの位置を使う)
    XMFLOAT3 world = transform.GetWorldTransform().position;
    XMFLOAT2 screen = Image::WorldToScreen2D({ world.x + offset.x, world.y + offset.y, world.z + offset.z });
    Text::Draw(text, screen.x, screen.y, style);
}
