#include "PlayerController.h"
#include "CameraController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"
#include <cmath>
#include <PxPhysicsAPI.h>

void PlayerController::Update(float dt) {

    // 必要なComponentを取得(無ければ何もしない)
    Transform& t = GetOwner()->transform;
    MeshRenderer* renderer = GetOwner()->GetComponent<MeshRenderer>(); if (!renderer) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>(); if (!rigidbody) return;
    if (!camera) return;

    // 一人称の間は自分の見た目を隠す(カメラが立方体の内側に入り、内側の面で画面が埋まるため)。当たり判定は残す
    renderer->SetEnabled(camera->GetMode() == CameraController::Mode::ThirdPerson);

    // 無敵時間
    if (invincibleTimer > 0.f) invincibleTimer -= dt;

    // 色の初期化
    if (!touchingEnemy) renderer->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    touchingEnemy = false;

    // Y速度は0にせず、今の値をそのまま使う(0にすると重力で落ちなくなる)
    float velocityY = rigidbody->GetVelocity().y;

    // HPが0なら操作を受け付けない(滑り続けないよう水平の速度も0にする)
    if (hp <= 0) {
        rigidbody->SetVelocity({ 0.f, velocityY, 0.f });
        return;
    }

    // カメラと同じ向きを向く
    // rotate.y(度)のとき、正面(ローカルの+Z)は (sin, 0, cos) の方向になる。CameraControllerのyawと同じ式
    float yaw = camera->GetYaw();
    t.rotate.y = DirectX::XMConvertToDegrees(yaw);

    // 当たり判定の箱も同じ向きに回す(向きが変わった時だけ。位置はPhysXの今の位置のまま)
    // ※SetFreezeRotation(true)の剛体は、RigidbodyComponentが位置しか同期しないため、ここで回す。
    //   DirectXのY軸回転とPhysXのPxQuat(角度, Y軸)は、同じ角度で同じ向きに回る(+Zが(sin, 0, cos)へ)ので、
    //   そのままの角度で作る(EngineのToPxQuatと同じ考え方。ToPxQuatはEngineの中だけの関数なので、ここで直接作る)
    if (t.rotate.y != colliderRotateY) {
        if (physx::PxRigidActor* actor = rigidbody->GetActor()) {
            physx::PxTransform pose = actor->getGlobalPose();
            pose.q = physx::PxQuat(yaw, physx::PxVec3(0.f, 1.f, 0.f));
            actor->setGlobalPose(pose);
            colliderRotateY = t.rotate.y;
        }
    }

    // 移動処理: カメラ基準(W=カメラの前、D=カメラの右)で、地面と水平に動く
    DirectX::XMFLOAT3 forward = { sinf(yaw), 0.f, cosf(yaw) };
    DirectX::XMFLOAT3 right = { cosf(yaw), 0.f, -sinf(yaw) };
    float moveForward = 0.f;
    float moveRight = 0.f;
    if (Input::GetKeyPress(KEY_W)) moveForward += 1.f;
    if (Input::GetKeyPress(KEY_S)) moveForward -= 1.f;
    if (Input::GetKeyPress(KEY_D)) moveRight += 1.f;
    if (Input::GetKeyPress(KEY_A)) moveRight -= 1.f;

    // 斜め移動で速くならないよう、長さを1にそろえる
    float len = sqrtf(moveForward * moveForward + moveRight * moveRight);
    if (len > 0.f) {
        moveForward /= len;
        moveRight /= len;
    }

    // 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す
    DirectX::XMFLOAT3 velocity = {
        (forward.x * moveForward + right.x * moveRight) * speed,
        velocityY,
        (forward.z * moveForward + right.z * moveRight) * speed
    };
    rigidbody->SetVelocity(velocity);

    // 弾を生成(カーソル固定中だけ。カーソルを固定するための左クリックでは撃たない)
    if (Input::GetKeyDown(MOUSE_LEFT) && camera->IsCursorLocked() && !camera->IsLockedThisFrame()) {
        EventBus::Get().PublishObject("FireBullet", GetOwner());
    }
}

void PlayerController::OnCollisionStay2D(CollisionInfo info) {
    if (info.other->GetTag() == "Enemy") {
        touchingEnemy = true;

        MeshRenderer* renderer = GetOwner()->GetComponent<MeshRenderer>();
        if (renderer) renderer->SetColor(1.0f, 1.0f, 0.0f, 1.0f);

        // OnCollisionStay2Dは接触中ずっと毎フレーム呼ばれるので、無敵時間で連続ダメージを防ぐ
        if (hp <= 0 || invincibleTimer > 0.f) return;
        hp--;
        invincibleTimer = invincibleDuration;

        // ゲームオーバー
        if (hp <= 0) EventBus::Get().Publish("PlayerDied");
    }
}
