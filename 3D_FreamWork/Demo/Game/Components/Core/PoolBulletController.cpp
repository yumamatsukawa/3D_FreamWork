#include "PoolBulletController.h"
#include "../../../../Engine/Core/GameObject.h"

void PoolBulletController::Launch(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT2& speed, float lifeTime) {
    // 使い回した弾には前回の値が残っているので、毎回すべて決め直す
    GetOwner()->transform.position = position;
    velocity = speed;
    remainingLife = lifeTime;
}

void PoolBulletController::Update(float dt) {
    // 移動処理
    Transform& t = GetOwner()->transform;
    t.position.x += velocity.x * dt;
    t.position.y += velocity.y * dt;

    // 寿命: 時間が来たらプールに返す(返すと非アクティブになり、このUpdateは呼ばれなくなる)
    remainingLife -= dt;
    if (remainingLife <= 0.f && onExpire) onExpire(GetOwner());
}
