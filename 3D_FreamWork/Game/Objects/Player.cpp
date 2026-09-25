#include "Player.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/SpriteRenderer.h"
#include "../../Engine/RigidbodyComponent.h"
#include "../Components/PlayerController.h"
#include "../Components/CameraController.h"

GameObject* CreatePlayer(Scene& scene) {
    GameObject* player = scene.CreateObject("Player", "Player");
    player->transform.position = {   0.0f,   0.0f,   0.0f };
    player->transform.scale    = { 100.0f, 100.0f,   1.0f };

    // 見た目(3Dオブジェクト(cube)と正しく前後関係が出るよう、World空間モードにする)
    auto* sprite = player->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);
    sprite->SetWorldSpace(true);
    // X/Zの傾きは固定し、水平方向(Y軸)だけカメラに向く「立て看板」ビルボードにする
    sprite->SetBillboardLockY(false);

    auto* rigidbody = player->AddComponent<BoxRigidbodyComponent>(BodyType::Dynamic, player->transform.scale, 100.f);
    rigidbody->SetUseGravity(true);
    rigidbody->SetFreezeRotation(true);

    // 動き・入力操作
    player->AddComponent<PlayerController>();

    // 3Dカメラ(TABキーで三人称/一人称切り替え、右クリックドラッグで視点回転)
    player->AddComponent<CameraController>();

    return player;
}
