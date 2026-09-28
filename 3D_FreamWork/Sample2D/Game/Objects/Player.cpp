#include "Player.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../Components/PlayerController.h"

GameObject* CreatePlayer(Scene& scene) {
    GameObject* player = scene.CreateObject("Player", "Player");
    player->transform.position = {   0.0f,   0.0f,   0.0f };
    player->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // 見た目(トップダウン視点なので、スプライトはワールド空間モードで真上から見える形にする)
    auto* sprite = player->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);

    // トップダウンの2D操作なので、重力は不要(床が無くても落下しない)
    auto* rigidbody = player->AddComponent<SquareRigidbodyComponent>(BodyType::Dynamic, 100.0f, 100.f);
    rigidbody->SetUseGravity(false);
    rigidbody->SetFreezeRotation(true);
    rigidbody->SetFreezePositionZ(true);

    // 動き・入力操作(カメラ追従もPlayerController内で行う、トップダウンのシンプルな追従カメラ)
    player->AddComponent<PlayerController>();

    return player;
}
