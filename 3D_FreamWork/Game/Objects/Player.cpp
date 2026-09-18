#include "Player.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/SpriteRenderer.h"
#include "../../Engine/ColliderComponent.h"
#include "../Components/PlayerController.h"

GameObject* CreatePlayer(Scene& scene) {
    GameObject* player = scene.CreateObject("Player", "Player");
    player->transform.position = {   0.0f,   0.0f, 0.0f };
    player->transform.scale    = { 100.0f, 100.0f, 1.0f };

    // 見た目
    auto* sprite = player->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);

    // 動き・入力操作
    player->AddComponent<PlayerController>();

    // 当たり判定(すり抜ける円形)
    player->AddComponent<CircleColliderComponent>(true, 50.0f);

    return player;
}
