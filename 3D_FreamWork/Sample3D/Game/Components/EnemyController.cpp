#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/Audio.h"
#include "../Objects/Explosion.h"

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    // もう倒されている(プールに返した後)なら何もしない(同じフレームに2発当たっても、倒した知らせは1回だけ)
    if (!GetOwner()->GetIsActive()) return;

    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");

        // 倒した音(倒された場所で鳴らす)
        Audio::PlayOneShotAt(L"Assets/Sounds/hit.wav", GetOwner()->transform.position);

        // 倒された位置に爆発を出す(プールに返す前に、今の位置で)
        if (Scene* scene = GetOwner()->GetScene()) CreateExplosion(*scene, GetOwner()->transform.position);

        pool->Return(GetOwner());
    }
}
