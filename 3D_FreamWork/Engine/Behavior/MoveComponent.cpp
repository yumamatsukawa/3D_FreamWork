#include "MoveComponent.h"
#include "../Core/GameObject.h"
#include "../Physics/RigidbodyComponent.h"
#include "../Core/MathUtil.h"
#include <windows.h>
#include <cstdio>

void MoveComponent::SetTargetPosition(XMFLOAT3 position) {
    targetPosition = position;
    hasTargetPosition = true;
}

bool MoveComponent::HasArrived() const {
    XMFLOAT3 offset;
    if (!GetOffsetToDestination(offset)) return false;
    return IsWithinStopDistance(offset);
}

bool MoveComponent::GetOffsetToDestination(XMFLOAT3& offset) const {
    // 目的地: 相手がいればその位置、いなければSetTargetPositionの位置
    XMFLOAT3 destination;
    if (target) {
        if (!target->GetIsActive() || target->GetIsDestroyed()) return false;   // 非アクティブな相手は追わない
        destination = target->transform.position;
    }
    else if (hasTargetPosition) {
        destination = targetPosition;
    }
    else {
        return false;
    }
    offset = ToPlane(destination - GetOwner()->transform.position);
    return true;
}

bool MoveComponent::IsWithinStopDistance(XMFLOAT3 offset) const {
    return Math::Dot(offset, offset) < stopDistance * stopDistance;   // 2乗どうしで比べる(sqrt不要)
}

XMFLOAT3 MoveComponent::ToPlane(XMFLOAT3 v) const {
    if (plane == MovePlane::XY) v.z = 0.f;
    else v.y = 0.f;
    return v;
}

RigidbodyComponent* MoveComponent::GetDynamicRigidbody() const {
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
    if (rigidbody && rigidbody->GetBodyType() == BodyType::Dynamic) return rigidbody;
    return nullptr;
}

void MoveComponent::ApplyVelocity(XMFLOAT3 velocity, float dt) {
    velocity = ToPlane(velocity);

    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
    if (rigidbody) {
        switch (rigidbody->GetBodyType()) {
        case BodyType::Dynamic:
            // Dynamicは位置を直接書き換えず、PhysXの剛体に速度を渡す。
            // XZの時はYの速度(重力で落ちる速さ)を今の値のまま残す(0にすると落ちなくなる)
            if (plane == MovePlane::XZ) velocity.y = rigidbody->GetVelocity().y;
            rigidbody->SetVelocity(velocity);
            return;
        case BodyType::Static:
            // Staticは動かせない(毎フレーム出力ウィンドウが埋まらないように、1回だけ知らせる)
            if (!warnedStatic) {
                char message[256];
                sprintf_s(message, "★ MoveComponent: Staticの剛体は動かせません(%s)\n", GetOwner()->GetName().c_str());
                OutputDebugStringA(message);
                warnedStatic = true;
            }
            return;
        case BodyType::Kinematic:
        default:
            break;   // 下でtransformを動かす(RigidbodyComponentがPhysXへ反映する)
        }
    }

    // Kinematic・Rigidbody無し: transform.positionを直接動かす
    Transform& t = GetOwner()->transform;
    t.position = t.position + velocity * dt;
}
