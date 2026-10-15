#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../Objects/ScorePopup.h"

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");

        // 倒された位置に「+10」を出す(プールに返す前に、今の位置で)
        if (Scene* scene = GetOwner()->GetScene()) CreateScorePopup(*scene, GetOwner()->transform.position);

        pool->Return(GetOwner());
    }
}
