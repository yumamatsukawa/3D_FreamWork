#include "PlayerController.h"
#include "CameraController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/MathUtil.h"
#include "../../../Engine/ScreenEffect.h"
#include "../../../Engine/Time.h"
#include <PxPhysicsAPI.h>

void PlayerController::Init() {
    // 最初は無敵ではない(「終わった状態」にしておき、ダメージを受けたらResetで数え始める)
    invincibleTimer.Complete();
}

void PlayerController::Update(float dt) {

    // 必要なComponentを取得(無ければ何もしない)
    Transform& t = GetOwner()->transform;
    MeshRenderer* renderer = GetOwner()->GetComponent<MeshRenderer>(); if (!renderer) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>(); if (!rigidbody) return;
    if (!camera) return;

    // 一人称の間は自分の見た目を隠す(カメラが立方体の内側に入り、内側の面で画面が埋まるため)。当たり判定は残す
    renderer->SetEnabled(camera->GetMode() == CameraController::Mode::ThirdPerson);

    // ポーズ中・シーン切り替え中は操作を受け付けない(dtが0なのでタイマーも進まない)
    // ※見た目の切り替え(上)より後に置く。CameraControllerはポーズ中もTABで一人称にできるため
    if (Time::IsPaused()) return;

    // 無敵時間
    invincibleTimer.Update(dt);

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
    // 正面は上でカメラに合わせたrotate.yの向き、右は「上」と「正面」の外積
    DirectX::XMFLOAT3 forward = Math::Forward3D(t);
    DirectX::XMFLOAT3 right = Math::Cross({ 0.f, 1.f, 0.f }, forward);
    DirectX::XMFLOAT2 input = { 0.f, 0.f };   // x=右方向、y=前方向の入力
    if (Input::GetKeyPress(KEY_W)) input.y += 1.f;
    if (Input::GetKeyPress(KEY_S)) input.y -= 1.f;
    if (Input::GetKeyPress(KEY_D)) input.x += 1.f;
    if (Input::GetKeyPress(KEY_A)) input.x -= 1.f;

    // 斜め移動で速くならないよう、長さを1にそろえる(何も押していない時は0のまま)
    input = Math::Normalize(input);

    // 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す(Yは今の速度のまま)
    DirectX::XMFLOAT3 move = (forward * input.y + right * input.x) * speed;
    rigidbody->SetVelocity({ move.x, velocityY, move.z });

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
        if (hp <= 0 || !invincibleTimer.IsFinished()) return;
        hp--;
        invincibleTimer.Reset();

        // 画面の端を赤く点滅させる
        ScreenEffect::Vignette(damageVignetteColor, damageVignetteDuration, damageVignetteBlinkCount);

        // ゲームオーバー
        if (hp <= 0) EventBus::Get().Publish("PlayerDied");
    }
}
