#include "Player.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/CameraFollowComponent.h"
#include "../Components/PlayerController.h"

GameObject* CreatePlayer(Scene& scene) {
    GameObject* player = scene.CreateObject("Player", "Player");
    player->transform.position = {   0.0f,   0.0f,   0.0f };
    player->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // ※Componentは付けた順にUpdateされるので、順番を変えない:
    //   物理(PhysXの位置をtransformへ) → カメラ追従(その位置へカメラを動かす) → 操作(そのカメラでマウスのワールド座標を出す)

    // 見た目
    auto* sprite = player->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);

    // 当たり判定+物理演算(PhysX)。
    auto* rigidbody = player->AddComponent<CircleRigidbodyComponent>(BodyType::Dynamic, 50.0f, 100.f);
    rigidbody->SetUseGravity(false);      // 重力を適用するか
    rigidbody->SetFreezeRotation(true);   // trueでぶつかっても回転しない
    rigidbody->SetFreezePositionZ(true);  // trueでZ方向に動かない(X/Yにも同じ関数がある)

    // カメラがPlayerを追いかける(ピッタリ追従。SetSmoothTime(0.1f) などにすると少し遅れてなめらかに追う)
    player->AddComponent<CameraFollowComponent>();

    // 動き・入力操作
    player->AddComponent<PlayerController>();

    return player;
}
