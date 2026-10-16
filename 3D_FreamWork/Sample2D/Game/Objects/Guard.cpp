#include "Guard.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/SpriteRenderer.h"
#include "../../../Engine/VisionComponent.h"
#include "../Components/GuardController.h"

GameObject* CreateGuard(Scene& scene, GameObject* target) {
    GameObject* guard = scene.CreateObject("Guard");
    guard->transform.position = { -500.0f, 300.0f, 0.0f };
    guard->transform.scale    = {  100.0f, 100.0f, 100.0f };

    // 見た目(Playerと同じ画像。色は見えたかどうかで変わる)
    auto* sprite = guard->AddComponent<SpriteRenderer>(L"Assets/player.png", 8, 2);
    sprite->SetSpriteIndex(5);

    // 視野: 画面(XY平面)の上で、正面から左右45度ずつ(合わせて90度)・距離700。"Wall"のタグの物が視線をさえぎる
    auto* vision = guard->AddComponent<VisionComponent>();
    vision->SetFieldOfView(90.0f);
    vision->SetRange(700.0f);
    vision->SetPlane(MovePlane::XY);

    // 動き(視野と見た目を使うので、それらより後に付ける)
    auto* controller = guard->AddComponent<GuardController>();
    controller->Setup(target);

    return guard;
}