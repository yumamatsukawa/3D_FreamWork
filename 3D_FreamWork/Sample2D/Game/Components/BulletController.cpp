#include "BulletController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"

void BulletController::Update(float dt) {
    Transform& t = GetOwner()->transform;
    // ★ Sample2DはX/Y平面で動く(Z/上下移動は使わない)ので、direction.yはposition.yに適用する
    t.position.x += direction.x * speed * dt;
    t.position.y += direction.y * speed * dt;

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
