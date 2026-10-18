#include "AccelerateChaseComponent.h"
#include "../Core/GameObject.h"
#include "../Physics/RigidbodyComponent.h"
#include "../Core/MathUtil.h"

namespace {
    // 長さがmaxLengthを超えていたら、向きはそのままで長さをmaxLengthにする
    XMFLOAT3 ClampLength(XMFLOAT3 v, float maxLength) {
        float length = Math::Length(v);
        if (length <= maxLength || length < 1e-6f) return v;
        return v * (maxLength / length);
    }
}

void AccelerateChaseComponent::Update(float dt) {
    // 目的地が無ければ止まる
    XMFLOAT3 offset;
    if (!GetOffsetToDestination(offset)) {
        velocity = { 0.f, 0.f, 0.f };
        ApplyVelocity(velocity, dt);
        return;
    }

    // 着いていなければ目的地の方へ加速する(着いている間は加速しない → 勢いで行き過ぎて、また戻ってくる)
    XMFLOAT3 accel = { 0.f, 0.f, 0.f };
    if (!IsWithinStopDistance(offset)) accel = Math::Normalize(offset) * acceleration;

    // 空気抵抗: 1秒あたりdragの割合で遅くなる(dtが大きくても速度が逆向きにならない式)
    float dragFactor = 1.f / (1.f + drag * dt);

    // Dynamic: 今の速度(ぶつかって変わった分も含む)に空気抵抗と最高速度をかけてから、AddForceで加速する
    if (RigidbodyComponent* rigidbody = GetDynamicRigidbody()) {
        velocity = ClampLength(ToPlane(rigidbody->GetVelocity()) * dragFactor, speed);
        ApplyVelocity(velocity, dt);   // XZならYの速度(重力)は残る
        // ポーズ中(dt=0)はPhysXが進まず、加えた力が貯まり続けるので加えない
        if (dt > 0.f) rigidbody->AddForce(accel, ForceMode::Acceleration);
        return;
    }

    // Kinematic・Rigidbody無し: 自分で速度を持って積分する
    velocity = ClampLength((velocity + accel * dt) * dragFactor, speed);
    ApplyVelocity(velocity, dt);
}

void AccelerateChaseComponent::OnActiveChanged(bool active) {
    // ObjectPoolで再利用された時は、前回の勢いを残さない
    if (active) velocity = { 0.f, 0.f, 0.f };
}
