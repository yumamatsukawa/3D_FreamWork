#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/MathUtil.h"

namespace {
    const int walkFrames[] = { 4, 5 };   // 歩きアニメーションで使うコマの番号(この順に表示)
    constexpr int walkFrameCount = sizeof(walkFrames) / sizeof(walkFrames[0]);
}

void EnemyController::Update(float dt) {
    // 必要なComponentを取得(無ければ何もしない)
    if (!target) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>(); if (!rigidbody) return;
    SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>(); if (!sprite) return;

    // 歩きアニメーション: 1コマ分の時間が来る度に次のコマへ(余った時間はTimerが次へ持ち越す)
    if (frameTimer.Update(dt)) frameIndex = (frameIndex + 1) % walkFrameCount;
    sprite->SetSpriteIndex(walkFrames[frameIndex]);

    // ★ Y速度は0にせず、今の値をそのまま使う(0にすると重力で落ちなくなる)
    float velocityY = rigidbody->GetVelocity().y;

    // 自分とtargetの位置(地面と水平なXZ平面で考えるので、Yは0にそろえる)
    Transform& t = GetOwner()->transform;
    XMFLOAT3 from = { t.position.x, 0.f, t.position.z };
    XMFLOAT3 to = { target->transform.position.x, 0.f, target->transform.position.z };

    // ほぼ重なっている時は止まる(方向が定まらないため)
    if (Math::DistanceSq(from, to) < 1.0f) {
        rigidbody->SetVelocity({ 0.f, velocityY, 0.f });
        return;
    }

    // 移動処理: 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す
    XMFLOAT3 dir = Math::DirectionTo(from, to);
    rigidbody->SetVelocity({ dir.x * speed, velocityY, dir.z * speed });
}

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");
        pool->Return(GetOwner());
    }
}
