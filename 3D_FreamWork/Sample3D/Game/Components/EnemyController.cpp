#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/SpriteRenderer.h"

namespace {
    const int walkFrames[] = { 4, 5 };   // 歩きアニメーションで使うコマの番号(この順に表示)
    constexpr int walkFrameCount = sizeof(walkFrames) / sizeof(walkFrames[0]);
}

void EnemyController::Update(float dt) {
    // 必要なComponentを取得(無ければ何もしない)
    SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>(); if (!sprite) return;

    // 歩きアニメーション: 1コマ分の時間が来る度に次のコマへ(余った時間はTimerが次へ持ち越す)
    if (frameTimer.Update(dt)) frameIndex = (frameIndex + 1) % walkFrameCount;
    sprite->SetSpriteIndex(walkFrames[frameIndex]);
}

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");
        pool->Return(GetOwner());
    }
}
