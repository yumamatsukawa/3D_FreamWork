#include "BulletController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"

void BulletController::Update(float dt) {
    Transform& t = GetOwner()->transform;

    // まっすぐ進む(Kinematicなので位置を直接動かしてよい。重力の影響も受けない)
    t.position.x += direction.x * speed * dt;
    t.position.y += direction.y * speed * dt;
    t.position.z += direction.z * speed * dt;

    // 寿命が来たらプールに返す
    elapsed += dt;
    if (elapsed >= lifeTime && pool) {
        pool->Return(GetOwner());
    }
}

void BulletController::OnTriggerEnter2D(Collider2D* other) {
    // 敵に当たったら消える(プールに返す)
    if (other->GetTag() == "Enemy" && pool) {
        pool->Return(GetOwner());
    }
}
