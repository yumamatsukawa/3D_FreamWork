#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/Audio.h"

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");

        // 倒した音(倒された場所で鳴らす)
        Audio::PlayOneShotAt(L"Assets/Sounds/hit.wav", GetOwner()->transform.position);
        pool->Return(GetOwner());
    }
}
