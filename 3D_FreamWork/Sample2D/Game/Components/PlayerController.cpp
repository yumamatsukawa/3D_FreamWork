#include "PlayerController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/Input.h"
#include "../../../Engine/Collider.h"
#include "../../../Engine/Image.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/MathUtil.h"

void PlayerController::Init() {
    // 最初は無敵ではない(「終わった状態」にしておき、ダメージを受けたらResetで数え始める)
    invincibleTimer.Complete();
}

void PlayerController::Update(float dt) {

    // 必要なComponentを取得(無ければ何もしない)
    Transform& t = GetOwner()->transform;
    SpriteRenderer* renderer = GetOwner()->GetComponent<SpriteRenderer>(); if (!renderer) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>(); if (!rigidbody) return;

    // 無敵時間
    invincibleTimer.Update(dt);

    // 色の初期化
    if (!touchingEnemy) renderer->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    touchingEnemy = false;

    // カメラをプレイヤーに追従させる
    Camera& camera = Image::GetCamera();
    camera.position.x = t.position.x;
    camera.position.y = t.position.y;

    // HPが0なら操作を受け付けない(滑り続けないよう速度も0にする)
    if (hp <= 0) {
        if (rigidbody) rigidbody->SetVelocity({ 0.f, 0.f, 0.f });
        return;
    }

    // 移動処理: 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す
    DirectX::XMFLOAT3 velocity = { 0.f, 0.f, 0.f };
    if (Input::GetKeyPress(KEY_W)) velocity.y += speed;
    if (Input::GetKeyPress(KEY_S)) velocity.y -= speed;
    if (Input::GetKeyPress(KEY_A)) velocity.x -= speed;
    if (Input::GetKeyPress(KEY_D)) velocity.x += speed;
    if (rigidbody) rigidbody->SetVelocity(velocity);

    // マウスの方を向く
    // カメラは上で先に動かしてあるので、このフレームのカメラ位置で計算される
    DirectX::XMFLOAT2 mouseWorldPos = Input::GetMouseWorldPosition();
    DirectX::XMFLOAT2 playerPos = Math::ToXY(t.position);
    if (Math::DistanceSq(playerPos, mouseWorldPos) > 1.0f) {  // マウスがほぼ真上にある時は向きが定まらないので変えない
        t.rotate.z = Math::AngleToTarget2D(playerPos, mouseWorldPos);
    }

    // 弾を生成
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        EventBus::Get().PublishObject("FireBullet", GetOwner());
    }
}

void PlayerController::OnCollisionStay2D(CollisionInfo info) {
    if (info.other->GetTag() == "Enemy") {
        touchingEnemy = true;

        SpriteRenderer* renderer = GetOwner()->GetComponent<SpriteRenderer>();
        if (renderer) renderer->SetColor(1.0f, 1.0f, 0.0f, 1.0f);

        // OnCollisionStay2Dは接触中ずっと毎フレーム呼ばれるので、無敵時間で連続ダメージを防ぐ
        if (hp <= 0 || !invincibleTimer.IsFinished()) return;
        hp--;
        invincibleTimer.Reset();

        // ゲームオーバー
        if (hp <= 0) EventBus::Get().Publish("PlayerDied");
    }
}
