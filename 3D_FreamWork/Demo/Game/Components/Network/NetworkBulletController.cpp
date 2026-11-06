#include "NetworkBulletController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Network/Network.h"

void NetworkBulletController::Launch() {
    velocity = { 0.f, speed, 0.f };   // 真上に飛ぶ(2Dも3Dも同じ)
    life = lifetime;
}

void NetworkBulletController::Update(float dt) {
    // 動かして消すのはホストだけ(クライアントの弾は、位置を受け取って表示するだけ。消えるのも Despawn が届いてから)
    NetworkIdentityComponent* identity = GetOwner()->GetComponent<NetworkIdentityComponent>();
    if (!identity || !identity->HasAuthority()) return;

    // 移動処理
    Transform& transform = GetOwner()->transform;
    transform.position.x += velocity.x * dt;
    transform.position.y += velocity.y * dt;
    transform.position.z += velocity.z * dt;

    // 寿命: 尽きたら全員の画面から消す(プールに返される)
    life -= dt;
    if (life <= 0.f) Network::Despawn(GetOwner());
}