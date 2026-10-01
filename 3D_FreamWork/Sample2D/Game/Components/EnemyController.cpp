#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"
#include <cmath>

void EnemyController::Update(float dt) {
    if (!target) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
    if (!rigidbody) return;

    // Dynamicなので、位置ではなく速度を渡して追いかける
    // (位置を直接書き換えても、次のフレームでPhysX側の位置に戻されてしまう)
    Transform& t = GetOwner()->transform;
    float dx = target->transform.position.x - t.position.x;
    float dy = target->transform.position.y - t.position.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) {  // ほぼ重なっているなら、これ以上近づかない(震え防止)
        rigidbody->SetVelocity({ 0.f, 0.f, 0.f });
        return;
    }
    rigidbody->SetVelocity({ (dx / len) * speed, (dy / len) * speed, 0.f });
}

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");
        pool->Return(GetOwner());
    }
}
