#include "Player.h"
#include "../../../Engine/Core/Scene.h"
#include "../../../Engine/Core/GameObject.h"
#include "../../../Engine/Render/SpriteRenderer.h"
#include "../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../Engine/Render/CameraFollowComponent.h"
#include "../../../Engine/UI/GaugeRenderer.h"
#include "../../../Engine/Audio/AudioListenerComponent.h"
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
    sprite->SetMode(SpriteMode::World2D);   // 2Dモード: zで大きさは変わらず、zが小さいほど手前(Playerはz=0。敵(+5)の手前、弾(-5)の奥)

    // 当たり判定+物理演算(PhysX)。
    auto* rigidbody = player->AddComponent<CircleRigidbodyComponent>(BodyType::Dynamic, 50.0f, 100.f);
    rigidbody->SetUseGravity(false);      // 重力を適用するか
    rigidbody->SetFreezeRotation(true);   // trueでぶつかっても回転しない
    rigidbody->SetFreezePositionZ(true);  // trueでZ方向に動かない(X/Yにも同じ関数がある)

    // カメラがPlayerを追いかける(ピッタリ追従。SetSmoothTime(0.1f) などにすると少し遅れてなめらかに追う)
    player->AddComponent<CameraFollowComponent>();

    // 音を聞く位置(Playerの位置で聞こえる。敵や弾の音が、Playerからの距離と左右の向きで聞こえ方が変わる)
    player->AddComponent<AudioListenerComponent>();

    // 動き・入力操作
    auto* controller = player->AddComponent<PlayerController>();

    // 頭上のHPバー(Playerの上70の位置に出る。値は持たず、PlayerControllerのHPを毎フレーム読んで表示するだけ)
    auto* gauge = player->AddComponent<GaugeRenderer>();
    gauge->SetFollowTarget(player, { 0.f, 70.f });
    gauge->SetSize(80.f, 10.f);
    gauge->SetDelayedBar(true, { 1.0f, 0.9f, 0.4f, 1.0f });   // 減った分が少し遅れて縮む
    gauge->SetValueSource(
        [controller] { return static_cast<float>(controller->GetHp()); },
        [controller] { return static_cast<float>(controller->GetMaxHp()); });

    return player;
}
