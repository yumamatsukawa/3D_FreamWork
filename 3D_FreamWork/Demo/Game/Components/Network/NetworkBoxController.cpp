#include "NetworkBoxController.h"
#include "NetworkPlayerInputController.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Network/Network.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include <cmath>

void NetworkBoxController::Update(float dt) {
    NetworkIdentityComponent* identity = GetOwner()->GetComponent<NetworkIdentityComponent>();
    if (!identity) return;
    const int player = identity->GetOwnerPlayerIndex();

    // 色: つながっている人の箱は明るく、いない人の箱は暗くする(どの画面でも名簿で分かる)
    const float shade = Network::IsPlayerConnected(player) ? 1.f : 0.25f;
    if (MeshRenderer* mesh = GetOwner()->GetComponent<MeshRenderer>()) mesh->SetColor(baseColor.x * shade, baseColor.y * shade, baseColor.z * shade, 1.f);
    if (SpriteRenderer* sprite = GetOwner()->GetComponent<SpriteRenderer>()) sprite->SetColor(baseColor.x * shade, baseColor.y * shade, baseColor.z * shade, 1.f);

    // ルールはホストだけ(クライアントの箱は、受け取った位置を表示するだけ)
    if (!identity->HasAuthority()) return;
    RigidbodyComponent* rigidbody = GetOwner()->GetComponent<RigidbodyComponent>();
    if (!rigidbody || rigidbody->GetBodyType() != BodyType::Dynamic) return;

    // 移動処理: そのプレイヤーの入力(ホスト自身もクライアントも同じ読み方)で、SetVelocityで動かす(Transformは直接書かない)
    const NetworkInput& input = Network::GetInput(player);
    if (is3D) {
        DirectX::XMFLOAT3 velocity = rigidbody->GetVelocity();
        // ジャンプ: ほぼ地面にいる時にボタンを押した瞬間
        if (input.WasButtonPressed(NetworkPlayerInputController::jumpButton) && std::fabs(velocity.y) < 5.f) velocity.y = jumpSpeed;
        rigidbody->SetVelocity({ input.moveX * speed, velocity.y, input.moveY * speed });
    }
    else {
        rigidbody->SetVelocity({ input.moveX * speed, input.moveY * speed, 0.f });
    }
}