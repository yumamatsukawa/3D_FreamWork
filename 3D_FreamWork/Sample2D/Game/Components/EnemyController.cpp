#include "EnemyController.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/ObjectPool.h"
#include "../../../Engine/EventBus.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/MathUtil.h"

void EnemyController::Update(float dt) {
    // 必要なComponentを取得(無ければ何もしない)
    if (!target) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
    if (!rigidbody) return;

    // 自分とtargetの位置(2DなのでX,Yだけで考える)
    XMFLOAT2 from = Math::ToXY(GetOwner()->transform.position);
    XMFLOAT2 to = Math::ToXY(target->transform.position);

    // ほぼ重なっている時は止まる(方向が定まらないため)
    if (Math::DistanceSq(from, to) < 1.0f) {
        rigidbody->SetVelocity({ 0.f, 0.f, 0.f });
        return;
    }

    // 移動処理: 位置を直接書き換えるのではなく、PhysXの剛体に速度を渡す
    XMFLOAT2 velocity = Math::DirectionTo(from, to) * speed;
    rigidbody->SetVelocity({ velocity.x, velocity.y, 0.f });
}

void EnemyController::OnTriggerEnter2D(Collider2D* other) {
    // 弾に当たったら倒される(スコア加算はGameManagerの仕事なので、ここでは知らせるだけ)
    if (other->GetTag() == "Bullet" && pool) {
        EventBus::Get().Publish("EnemyDefeated");
        pool->Return(GetOwner());
    }
}
