#include "CameraFollowComponent.h"
#include "../Core/GameObject.h"
#include "Image.h"
#include "../Core/MathUtil.h"
#include <cmath>

namespace {
    // なめらかに追う時の速さの係数。smoothTime秒で、残りの距離が e^-3 ≒ 5% になる
    constexpr float smoothRate = 3.f;

    // 1つの軸を範囲に収める。範囲が画面より狭い時は、範囲の真ん中に固定する
    float ClampAxis(float center, float minValue, float maxValue, float halfView) {
        if (maxValue - minValue <= halfView * 2.f) return (minValue + maxValue) * 0.5f;
        return Math::Clamp(center, minValue + halfView, maxValue - halfView);
    }
}

void CameraFollowComponent::Update(float dt) {
    Camera& camera = Image::GetCamera();

    // 追いかける点: 対象のワールド座標 + ずらす量
    XMFLOAT3 world = GetOwner()->transform.GetWorldTransform().position;
    XMFLOAT2 focus = { world.x + offset.x, world.y + offset.y };

    // デッドゾーン: 追いかける点が、goalを中心とした四角から出た分だけgoalを動かす(0なら常にgoal=追いかける点)
    if (snapNext) {
        goal = focus;
    }
    else {
        float halfW = deadZone.x * 0.5f;
        float halfH = deadZone.y * 0.5f;
        goal.x = Math::Clamp(goal.x, focus.x - halfW, focus.x + halfW);
        goal.y = Math::Clamp(goal.y, focus.y - halfH, focus.y + halfH);
    }
    goal = ClampToBounds(goal);

    // なめらかに追う: 残りの距離を1秒あたり一定の割合で縮める(指数的に近づくので、dtの刻み方が変わっても同じ所に近づく)
    XMFLOAT2 position = goal;
    if (!snapNext && smoothTime > 0.f) {
        XMFLOAT2 current = { camera.position.x, camera.position.y };
        float t = 1.f - expf(-smoothRate * dt / smoothTime);
        position = ClampToBounds(current + (goal - current) * t);
    }

    camera.position.x = position.x;
    camera.position.y = position.y;
    snapNext = false;
}

void CameraFollowComponent::OnActiveChanged(bool active) {
    // 戻ってきた時は、遠くから流れてこないように一気に合わせる
    if (active) snapNext = true;
}

void CameraFollowComponent::SetBounds(XMFLOAT2 minCorner, XMFLOAT2 maxCorner) {
    // 左下と右上が逆に渡されても使えるように並べ直す
    boundsMin = { fminf(minCorner.x, maxCorner.x), fminf(minCorner.y, maxCorner.y) };
    boundsMax = { fmaxf(minCorner.x, maxCorner.x), fmaxf(minCorner.y, maxCorner.y) };
    hasBounds = true;
}

void CameraFollowComponent::SetDeadZone(XMFLOAT2 size) {
    deadZone = { (size.x > 0.f) ? size.x : 0.f, (size.y > 0.f) ? size.y : 0.f };
}

XMFLOAT2 CameraFollowComponent::ClampToBounds(XMFLOAT2 center) const {
    if (!hasBounds) return center;

    // 画面の半分の大きさ(ワールドの単位)。画面の右上の角をワールド座標に直して求める
    const Camera& camera = Image::GetCamera();
    XMFLOAT3 corner = Image::ScreenToWorld2D({ Graphics::width * 0.5f, Graphics::height * 0.5f });
    float halfViewW = corner.x - camera.position.x;
    float halfViewH = corner.y - camera.position.y;

    return {
        ClampAxis(center.x, boundsMin.x, boundsMax.x, halfViewW),
        ClampAxis(center.y, boundsMin.y, boundsMax.y, halfViewH)
    };
}
