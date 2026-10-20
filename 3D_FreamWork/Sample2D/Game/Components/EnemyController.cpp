#include "EnemyController.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Core/ObjectPool.h"
#include "../../../Engine/Core/EventBus.h"
#include "../../../Engine/Audio/Audio.h"
#include "../Objects/ScorePopup.h"
#include "../Objects/Explosion.h"

void EnemyController::OnTriggerEnter(Collider2D* other) {
    // もう倒されている(プールに返した後)なら何もしない(同じフレームに2発当たっても、倒した知らせは1回だけ)
    if (!GetOwner()->GetIsActive()) return;

    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");

        // 倒した音(倒された場所で鳴らす)
        Audio::PlayOneShotAt(L"Assets/Sounds/hit.wav", GetOwner()->transform.position);

        // 倒された位置に「+10」を出す(プールに返す前に、今の位置で)
        // 倒された位置に爆発も出す
        if (Scene* scene = GetOwner()->GetScene()) {
            CreateScorePopup(*scene, GetOwner()->transform.position);
            CreateExplosion(*scene, GetOwner()->transform.position);
        }

        pool->Return(GetOwner());
    }
}
