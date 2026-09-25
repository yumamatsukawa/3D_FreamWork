#include "BulletController.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/ObjectPool.h"

void BulletController::Update(float dt) {
    Transform& t = GetOwner()->transform;
    t.position.x += direction.x * speed * dt;
    t.position.z += direction.y * speed * dt;

    elapsed += dt;
    if (elapsed >= lifeTime && pool) {
        pool->Return(GetOwner());
    }
}

void BulletController::OnTriggerEnter2D(Collider2D* other) {
    if (other->GetTag() == "Enemy" && pool) {
        pool->Return(GetOwner());
    }
}
