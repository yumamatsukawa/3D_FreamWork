#include "DemoWalkerController.h"
#include "DemoCameraController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <PxPhysicsAPI.h>
#include <cmath>

void DemoWalkerController::Respawn() {
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
    if (!rigidbody) return;

    // PhysXの剛体を、スタート地点へ移す(Transformを書き換えても、PhysXの位置で上書きされるため)
    if (physx::PxRigidActor* actor = rigidbody->GetActor()) {
        actor->setGlobalPose(physx::PxTransform(physx::PxVec3(spawnPosition.x, spawnPosition.y, spawnPosition.z)));
    }
    rigidbody->SetVelocity({ 0.f, 0.f, 0.f });
    GetOwner()->transform.position = spawnPosition;
}

void DemoWalkerController::Update(float dt) {
    // ポーズ中・シーン切り替え中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 必要なComponentを取得(無ければ何もしない)
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>(); if (!rigidbody) return;
    if (!camera) return;

    // スタート地点へ戻る: Rキー、または地形の外へ落ちた時
    if (Input::GetKeyDown(KEY_R) || GetOwner()->transform.position.y < spawnPosition.y - fallLimit) {
        Respawn();
        return;
    }

    // 移動処理: カメラ基準(W=カメラの前、D=カメラの右)で、地面と水平に動く
    const float yaw = camera->GetYaw();
    DirectX::XMFLOAT3 forward = { sinf(yaw), 0.f, cosf(yaw) };
    DirectX::XMFLOAT3 right = Math::Cross({ 0.f, 1.f, 0.f }, forward);
    DirectX::XMFLOAT2 input = { 0.f, 0.f };   // x=右方向、y=前方向の入力
    if (Input::GetKeyPress(KEY_W)) input.y += 1.f;
    if (Input::GetKeyPress(KEY_S)) input.y -= 1.f;
    if (Input::GetKeyPress(KEY_D)) input.x += 1.f;
    if (Input::GetKeyPress(KEY_A)) input.x -= 1.f;

    // 斜め移動で速くならないよう、長さを1にそろえる(何も押していない時は0のまま)
    input = Math::Normalize(input);

    // 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す(Yは今の速度のまま。0にすると重力で落ちなくなる)
    DirectX::XMFLOAT3 move = (forward * input.y + right * input.x) * speed;
    rigidbody->SetVelocity({ move.x, rigidbody->GetVelocity().y, move.z });
}
