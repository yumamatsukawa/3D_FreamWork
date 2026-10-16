#include "Guard.h"
#include "../../../Engine/Scene.h"
#include "../../../Engine/GameObject.h"
#include "../../../Engine/MeshRenderer.h"
#include "../../../Engine/VisionComponent.h"
#include "../Components/GuardController.h"

GameObject* CreateGuard(Scene& scene, GameObject* target) {
    // 地面(上面の高さ-50)の上に立つ立方体。Playerと同じ高さなので、目の高さもPlayerの中心と同じ
    GameObject* guard = scene.CreateObject("Guard");
    guard->transform.position = { -800.0f, 0.0f, 600.0f };
    guard->transform.scale    = {  100.0f, 100.0f, 100.0f };

    // 見た目(3Dメッシュの立方体)
    guard->AddComponent<MeshRenderer>(Mesh::CreateCube());

    // 視野: 地面(XZ平面)の上で、正面から左右45度ずつ(合わせて90度)・距離1500。"Wall"のタグの物が視線をさえぎる
    auto* vision = guard->AddComponent<VisionComponent>();
    vision->SetFieldOfView(90.0f);
    vision->SetRange(1500.0f);
    vision->SetPlane(MovePlane::XZ);

    // 動き(視野と見た目を使うので、それらより後に付ける)
    auto* controller = guard->AddComponent<GuardController>();
    controller->Setup(target);

    return guard;
}