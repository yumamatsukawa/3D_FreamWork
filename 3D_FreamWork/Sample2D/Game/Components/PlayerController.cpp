#include "PlayerController.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render/SpriteRenderer.h"
#include "../../../Engine/Core/Input.h"
#include "../../../Engine/Physics/Collider.h"
#include "../../../Engine/Render/CameraShake.h"
#include "../../../Engine/Render/ScreenEffect.h"
#include "../../../Engine/Core/EventBus.h"
#include "../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../Engine/Core/MathUtil.h"
#include "../../../Engine/Core/Time.h"

void PlayerController::Init() {
    // 最初は無敵ではない(「終わった状態」にしておき、ダメージを受けたらResetで数え始める)
    invincibleTimer.Complete();
    knockbackTimer.Complete();
}

void PlayerController::Update(float dt) {

    // ポーズ中・シーン切り替え中は操作を受け付けない(dtが0なのでタイマーも進まない)
    if (Time::IsPaused()) return;

    // 必要なComponentを取得(無ければ何もしない)
    Transform& t = GetOwner()->transform;
    SpriteRenderer* renderer = GetOwner()->GetComponent<SpriteRenderer>(); if (!renderer) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>(); if (!rigidbody) return;

    // 無敵時間・弾かれている時間
    invincibleTimer.Update(dt);
    knockbackTimer.Update(dt);

    // 色の初期化
    if (!touchingEnemy) renderer->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
    touchingEnemy = false;

    // HPが0なら操作を受け付けない(滑り続けないよう速度も0にする)
    if (hp <= 0) {
        if (rigidbody) rigidbody->SetVelocity({ 0.f, 0.f, 0.f });
        return;
    }

    // 移動処理: 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す
    // (弾かれている間は、AddForceで受けた勢いを消さないように速度を上書きしない)
    if (knockbackTimer.IsFinished()) {
        DirectX::XMFLOAT3 velocity = { 0.f, 0.f, 0.f };
        if (Input::GetKeyPress(KEY_W)) velocity.y += speed;
        if (Input::GetKeyPress(KEY_S)) velocity.y -= speed;
        if (Input::GetKeyPress(KEY_A)) velocity.x -= speed;
        if (Input::GetKeyPress(KEY_D)) velocity.x += speed;
        if (rigidbody) rigidbody->SetVelocity(velocity);
    }

    // マウスの方を向く
    // カメラはCameraFollowComponent(このComponentより先にUpdateされる)が動かしてあるので、このフレームのカメラ位置で計算される
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

void PlayerController::OnCollisionStay(CollisionInfo info) {
    if (info.other->GetTag() == "Enemy") {
        touchingEnemy = true;

        SpriteRenderer* renderer = GetOwner()->GetComponent<SpriteRenderer>();
        if (renderer) renderer->SetColor(1.0f, 1.0f, 0.0f, 1.0f);

        // OnCollisionStayは接触中ずっと毎フレーム呼ばれるので、無敵時間で連続ダメージを防ぐ
        if (hp <= 0 || !invincibleTimer.IsFinished()) return;
        hp--;
        invincibleTimer.Reset();

        // 画面を揺らして、画面の端を赤く点滅させる
        CameraShake::Shake(damageShakeStrength, damageShakeDuration);
        ScreenEffect::Vignette(damageVignetteColor, damageVignetteDuration, damageVignetteBlinkCount);

        // ゲームオーバー
        if (hp <= 0) EventBus::Get().Publish("PlayerDied");

        // ノックバック: 敵から離れる向き(XY平面)へ弾く。倒れた時は弾かない(Updateで止まるため)
        // Impulseは重さで割られるので、強さを「弾かれる速さ × 重さ」にする
        // ※ここはPhysXのコールバックの中だが、AddForceはPhysXの計算が終わった直後に加えてくれるので呼んでよい
        RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
        if (hp > 0 && rigidbody) {
            DirectX::XMFLOAT2 away = Math::DirectionTo(
                Math::ToXY(info.other->owner->transform.position), Math::ToXY(GetOwner()->transform.position));
            float strength = knockbackSpeed * rigidbody->GetMass();
            rigidbody->AddForce({ away.x * strength, away.y * strength, 0.f }, ForceMode::Impulse);
            knockbackTimer.Reset();
        }
    }
}
