#include "HomingComponent.h"
#include "../Core/GameObject.h"
#include "../Core/MathUtil.h"

void HomingComponent::Update(float dt) {
    Transform& t = GetOwner()->transform;

    // 出した後の最初のUpdateで、今の見た目の向きを進行方向にする
    if (!headingInitialized) {
        heading = (plane == MovePlane::XY) ? t.rotate.z : t.rotate.y;
        headingInitialized = true;
    }

    // 目的地が無い・着いた時は止まる
    XMFLOAT3 offset;
    if (!GetOffsetToDestination(offset) || IsWithinStopDistance(offset)) {
        ApplyVelocity({ 0.f, 0.f, 0.f }, dt);
        return;
    }

    // 曲がる: 目的地の方の角度へ、1フレームに最大 turnSpeed*dt 度だけ近づける
    float targetAngle = (plane == MovePlane::XY)
        ? Math::AngleToTarget2D({ 0.f, 0.f }, Math::ToXY(offset))
        : Math::AngleToTargetY({ 0.f, 0.f, 0.f }, offset);
    heading = Math::MoveTowardsAngle(heading, targetAngle, turnSpeed * dt);
    heading = Math::DeltaAngle(0.f, heading);   // -180〜180に収める(回り続けて数値が大きくならないように)

    // 見た目の向きも合わせる
    if (faceMovement) {
        if (plane == MovePlane::XY) t.rotate.z = heading;
        else t.rotate.y = heading;
    }

    // 移動処理: 進行方向へspeedで進む(向きはrotateと同じ式 = Math::Forward2D / Forward3D で求める)
    Transform headingTransform;
    XMFLOAT3 direction;
    if (plane == MovePlane::XY) {
        headingTransform.rotate.z = heading;
        XMFLOAT2 forward = Math::Forward2D(headingTransform);
        direction = { forward.x, forward.y, 0.f };
    }
    else {
        headingTransform.rotate.y = heading;
        direction = Math::Forward3D(headingTransform);
    }
    ApplyVelocity(direction * speed, dt);
}

void HomingComponent::OnActiveChanged(bool active) {
    // ObjectPoolで再利用された時は、次のUpdateで進行方向をrotateから読み直す
    if (active) headingInitialized = false;
}
