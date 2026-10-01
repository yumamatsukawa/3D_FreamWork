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

    // 動き・入力操作
    player->AddComponent<PlayerController>();

    // 見た目
    auto* sprite = player->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);

    // 当たり判定+物理演算(PhysX)。
    auto* rigidbody = player->AddComponent<CircleRigidbodyComponent>(BodyType::Dynamic, 50.0f, 100.f);
    rigidbody->SetUseGravity(false);      // 重力を適用するか
    rigidbody->SetFreezeRotation(true);   // ぶつかったときに回転するか
    rigidbody->SetFreezePositionZ(true);  // ぶつかったときに移動するか(x,y,zそれぞれある)

    return player;
}
