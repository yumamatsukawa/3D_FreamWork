#include "Player.h"
#include "../../Engine/Scene.h"
#include "../../Engine/GameObject.h"
#include "../../Engine/SpriteRenderer.h"
#include "../../Engine/ColliderComponent.h"
#include "../Components/PlayerController.h"
#include "../Components/CameraController.h"

GameObject* CreatePlayer(Scene& scene) {
    GameObject* player = scene.CreateObject("Player", "Player");
    player->transform.position = {   0.0f,   0.0f, 0.0f };
    player->transform.scale    = { 100.0f, 100.0f, 1.0f };

    // 見た目(3Dオブジェクト(cube)と正しく前後関係が出るよう、World空間モードにする)
    auto* sprite = player->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);
    sprite->SetWorldSpace(true);
    // X/Zの傾きは固定し、水平方向(Y軸)だけカメラに向く「立て看板」ビルボードにする
    sprite->SetBillboardLockX(false);
    sprite->SetBillboardLockY(false);

    // 動き・入力操作
    player->AddComponent<PlayerController>();

    // 当たり判定(すり抜ける円形)
    player->AddComponent<CircleColliderComponent>(false, 50.0f);

    // 3Dカメラ(TABキーで三人称/一人称切り替え、右クリックドラッグで視点回転)
    player->AddComponent<CameraController>();

    return player;
}
