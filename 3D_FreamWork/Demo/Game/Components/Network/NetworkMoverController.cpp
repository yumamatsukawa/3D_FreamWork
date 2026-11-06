#include "NetworkMoverController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Network/Network.h"
#include <cmath>

void NetworkMoverController::Update(float dt) {
    // 動かすのはホストだけ(オフラインでも動く)。クライアントは NetworkTransformComponent が位置を書く
    NetworkIdentityComponent* identity = GetOwner()->GetComponent<NetworkIdentityComponent>();
    if (!identity || !identity->HasAuthority()) return;

    // 移動処理: 円を描く。Kinematicの剛体なので、Transformを動かせば押された物が動く
    angle += angularSpeed * dt;
    Transform& transform = GetOwner()->transform;
    const float degrees = std::fmod(DirectX::XMConvertToDegrees(angle), 360.f);   // 0〜360を繰り返す(360度をまたぐ補間の確認用)
    if (is3D) {
        transform.position = { center.x + std::cos(angle) * radius, center.y, center.z + std::sin(angle) * radius };
        transform.rotate.y = degrees;
    }
    else {
        transform.position = { center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius, center.z };
        transform.rotate.z = degrees;
    }
}