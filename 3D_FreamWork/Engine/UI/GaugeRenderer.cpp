#include "GaugeRenderer.h"
#include "../Core/GameObject.h"
#include "../Render/Image.h"

using namespace DirectX;

void GaugeRenderer::SetValue(float current, float max) {
    // 割合(最大値が0以下・NaNなら空。0〜1に収める)
    float newRatio = (max > 0.f) ? current / max : 0.f;
    if (!(newRatio > 0.f)) newRatio = 0.f;
    if (newRatio > 1.f) newRatio = 1.f;

    // 遅れて縮む帯: 最初の値や、帯を使わない時は、中身と同じ。減った時は、少し待ってから縮め始める
    if (!hasValue || !delayedEnabled) {
        delayedRatio = newRatio;
    } else if (newRatio < ratio) {
        delayedHold = delayedHoldTime;
    }

    // 増えた時(帯より大きくなった時)は、帯もすぐに追いつく
    if (newRatio >= delayedRatio) delayedRatio = newRatio;

    ratio = newRatio;
    hasValue = true;
}

void GaugeRenderer::SetValueSource(std::function<float()> currentGetter, std::function<float()> maxGetter) {
    getCurrent = std::move(currentGetter);
    getMax = std::move(maxGetter);

    // 最初のDrawまでに読んでおく(最初の値から縮む動きが始まらないように)
    if (getCurrent && getMax) SetValue(getCurrent(), getMax());
}

void GaugeRenderer::SetColors(XMFLOAT4 fill, XMFLOAT4 background) {
    fillColor = fill;
    backgroundColor = background;
}

void GaugeRenderer::SetDelayedBar(bool enabled, XMFLOAT4 color) {
    // 使い始める時は、今の中身と同じ位置から始める(昔の値から縮み出さないように)
    if (enabled && !delayedEnabled) {
        delayedRatio = ratio;
        delayedHold = 0.f;
    }
    delayedEnabled = enabled;
    delayedColor = color;
}

void GaugeRenderer::SetSize(float width, float height) {
    if (!(width > 0.f) || !(height > 0.f)) return;   // 0以下(やNaN)は無視
    size = { width, height };
}

void GaugeRenderer::SetAnchor(Anchor anchorPoint, XMFLOAT2 offset) {
    positionMode = PositionMode::Anchored;
    anchor = anchorPoint;
    anchorOffset = offset;
}

void GaugeRenderer::SetFollowTarget(GameObject* followTarget, XMFLOAT2 offset) {
    positionMode = PositionMode::FollowTarget;
    target = followTarget;
    targetOffset = offset;
}

bool GaugeRenderer::GetScreenCenter(XMFLOAT2& center) const {
    // 画面に固定
    if (positionMode == PositionMode::Anchored) {
        center = GetAnchoredCenter(anchor, anchorOffset, size);
        return true;
    }

    // オブジェクトの上: targetが無い・非アクティブ・Destroyされた後なら描かない
    if (!target || !target->GetIsActive() || target->GetIsDestroyed()) return false;

    if (use3DCamera) {
        // 3D: 3D用カメラで画面座標に直す(カメラの後ろ=zが0以下なら描かない)
        XMFLOAT3 world;
        XMStoreFloat3(&world, XMVector3TransformCoord(XMVectorZero(), target->transform.GetWorldMatrix()));
        XMFLOAT3 screen = Image::WorldToScreen3D({ world.x + targetOffset.x, world.y + targetOffset.y, world.z });
        if (screen.z <= 0.f) return false;
        center = { screen.x, screen.y };
        return true;
    }

    // 2D: 2D用カメラで画面座標に直す(SpriteRendererと同じく、親と合わせたワールドの位置を使う)
    XMFLOAT3 world = target->transform.GetWorldTransform().position;
    center = Image::WorldToScreen2D({ world.x + targetOffset.x, world.y + targetOffset.y, world.z });
    return true;
}

void GaugeRenderer::Update(float dt) {
    // 値の持ち主から読む
    if (getCurrent && getMax) SetValue(getCurrent(), getMax());

    // 遅れて縮む帯: 少し待ってから、中身に追いつくまで縮める
    if (delayedRatio > ratio) {
        if (delayedHold > 0.f) {
            delayedHold -= dt;
        } else {
            delayedRatio -= delayedShrinkSpeed * dt;
            if (delayedRatio < ratio) delayedRatio = ratio;
        }
    }
}

void GaugeRenderer::Draw() {
    XMFLOAT2 center;
    if (!GetScreenCenter(center)) return;

    // 背景(ゲージ全体)
    Image::DrawRect(center, size, backgroundColor);

    // 左端を基準に、左から右へ伸ばす
    const float left = center.x - size.x * 0.5f;

    // 遅れて縮む帯(中身より先に描いて、中身の右側にはみ出した分だけが見える)
    if (delayedEnabled && delayedRatio > 0.f) {
        const float width = size.x * delayedRatio;
        Image::DrawRect({ left + width * 0.5f, center.y }, { width, size.y }, delayedColor);
    }

    // 中身
    if (ratio > 0.f) {
        const float width = size.x * ratio;
        Image::DrawRect({ left + width * 0.5f, center.y }, { width, size.y }, fillColor);
    }
}
