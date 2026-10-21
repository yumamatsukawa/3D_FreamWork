#include "BuoyancyController.h"
#include "../Common/DemoBodyReset.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../../Engine/Render3D/WaterRenderer.h"

namespace {
    constexpr float gravity = 981.f;                      // 重力の強さ(Physicsの重力と同じ)
    constexpr float buoyAcceleration = gravity * 2.f;     // 全部沈んだ時の浮力(加速度)。重力の2倍=球は水の半分の重さで、半分沈んで浮く
    constexpr float waterDrag = 6.f;                      // 水の抵抗(1秒あたり、速さが減る割合)。揺れが長引かない強さ
    constexpr float fallLimitY = -1000.f;                 // これより低く落ちたら、最初の位置へ戻す
}

void BuoyancyController::Update(float dt) {
    if (!water) return;
    if (!rigidbody) rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
    if (!rigidbody) return;

    // 遠くへ落ちた(水面の外へ出た)時は、最初の位置へ戻す
    Transform& t = GetOwner()->transform;
    if (t.position.y < fallLimitY) {
        ResetDemoBody(GetOwner(), startPosition);
        return;
    }

    // 水面の外は浮力なし(重力のまま落ちる)
    if (!water->Contains(t.position.x, t.position.z)) return;

    // 沈んだ深さ: 球の真下の波の高さ − 球の底(0 = 水面の上)。全部沈んだ深さ(直径)までで頭打ち
    float surfaceY = water->GetHeightAt(t.position.x, t.position.z);
    float depth = Math::Clamp(surfaceY - (t.position.y - radius), 0.f, radius * 2.f);
    if (depth <= 0.f) return;

    // 浮力: 沈んだ割合に比例した、上向きの加速度(質量に関係なく同じ強さ)
    float submerged = depth / (radius * 2.f);
    float lift = buoyAcceleration * submerged;

    // 水の抵抗: 速さと逆向きに、速さに比例した加速度(水に入りかけの時はゆるめる)
    DirectX::XMFLOAT3 velocity = rigidbody->GetVelocity();
    float wet = Math::Clamp(depth / radius, 0.f, 1.f);
    float drag = waterDrag * wet;
    rigidbody->AddForce({ -velocity.x * drag, lift - velocity.y * drag, -velocity.z * drag }, ForceMode::Acceleration);
}
