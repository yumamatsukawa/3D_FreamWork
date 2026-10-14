#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/SpriteRenderer.h"
#include <cmath>

namespace {
    const int walkFrames[] = { 4, 5 };   // 歩きアニメーションで使うコマの番号(この順に表示)
    constexpr int walkFrameCount = sizeof(walkFrames) / sizeof(walkFrames[0]);
}

void EnemyController::Update(float dt) {
    // 必要なComponentを取得(無ければ何もしない)
    if (!target) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>(); if (!rigidbody) return;
    SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>(); if (!sprite) return;

    animationTimer += dt;
    float loopTime = frameDuration * walkFrameCount;
    if (animationTimer >= loopTime) animationTimer -= loopTime;   // 余った時間は捨てずに次の周へ
    int frame = (int)(animationTimer / frameDuration);
    sprite->SetSpriteIndex(walkFrames[frame]);

    // ★ Y速度は0にせず、今の値をそのまま使う(0にすると重力で落ちなくなる)
    float velocityY = rigidbody->GetVelocity().y;

    // targetへの方向と距離(地面と水平なXZ平面で考える)
    Transform& t = GetOwner()->transform;
    float dx = target->transform.position.x - t.position.x;
    float dz = target->transform.position.z - t.position.z;
    float len = sqrtf(dx * dx + dz * dz);

    // ほぼ重なっている時は止まる(方向が定まらない・0で割ってしまうため)
    if (len < 1.0f) {
        rigidbody->SetVelocity({ 0.f, velocityY, 0.f });
        return;
    }

    // 移動処理: 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す
    rigidbody->SetVelocity({ (dx / len) * speed, velocityY, (dz / len) * speed });
}

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");
        pool->Return(GetOwner());
    }
}
