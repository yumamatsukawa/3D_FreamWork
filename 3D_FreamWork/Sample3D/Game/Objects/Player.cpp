#include "Player.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/RigidbodyComponent.h"
#include "../../../Engine/GaugeRenderer.h"
#include "../../../Engine/AudioListenerComponent.h"
#include "../Components/PlayerController.h"
#include "../Components/CameraController.h"

GameObject* CreatePlayer(Scene& scene) {
    GameObject* player = scene.CreateObject("Player", "Player");
    player->transform.position = {   0.0f,   0.0f,   0.0f };
    player->transform.scale    = { 100.0f, 100.0f, 100.0f };

    // 見た目(3Dメッシュの立方体)
    player->AddComponent<MeshRenderer>(Mesh::CreateCube());

    // 当たり判定+物理演算(PhysX)
    auto* rigidbody = player->AddComponent<BoxRigidbodyComponent>(BodyType::Dynamic, player->transform.scale, 100.f);
    rigidbody->SetUseGravity(true);
    // ぶつかっても回転しない。見た目の向き(rotate.y)もPhysXに上書きされず、PlayerControllerの値がそのまま効く
    rigidbody->SetFreezeRotation(true);
    // ※このままだと当たり判定の箱は回らないので、PlayerControllerがrotate.yに合わせて箱も回している

    // 3Dカメラ(TABキーで三人称/一人称切り替え、マウス移動で視点回転。ESCでカーソル解放、左クリックで再固定)。
    // PlayerControllerより先に付けて、このフレームのカメラの向きを使えるようにする
    auto* camera = player->AddComponent<CameraController>();

    // 音を聞く位置(Playerの位置で、向きは3D用カメラに合わせる)
    player->AddComponent<AudioListenerComponent>()->SetUse3DCamera(true);

    // 動き・入力操作(カメラの向きを基準にする)
    auto* controller = player->AddComponent<PlayerController>();
    controller->Setup(camera);

    // 頭上のHPバー(Playerの上80の位置に出る。3Dのオブジェクトなので3D用カメラで画面座標に直す)。
    // 値は持たず、PlayerControllerのHPを毎フレーム読んで表示するだけ。
    // 一人称の間はカメラがPlayerの位置にあって意味が無いので、PlayerControllerが見た目と一緒に隠す
    auto* gauge = player->AddComponent<GaugeRenderer>();
    gauge->SetUse3DCamera(true);
    gauge->SetFollowTarget(player, { 0.f, 80.f });
    gauge->SetSize(80.f, 10.f);
    gauge->SetDelayedBar(true, { 1.0f, 0.9f, 0.4f, 1.0f });   // 減った分が少し遅れて縮む
    gauge->SetValueSource(
        [controller] { return static_cast<float>(controller->GetHp()); },
        [controller] { return static_cast<float>(controller->GetMaxHp()); });

    return player;
}
