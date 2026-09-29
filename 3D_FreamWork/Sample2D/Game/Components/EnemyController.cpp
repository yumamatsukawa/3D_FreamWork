#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include <cmath>

void EnemyController::Update(float dt) {
    if (!target) return;

    Transform& t = GetOwner()->transform;
    float dx = target->transform.position.x - t.position.x;
    float dy = target->transform.position.y - t.position.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;  // ほぼ重なっているなら、これ以上近づかない(震え防止)

    t.position.x += (dx / len) * speed * dt;
    t.position.y += (dy / len) * speed * dt;
}

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");
        pool->Return(GetOwner());
    }
}
