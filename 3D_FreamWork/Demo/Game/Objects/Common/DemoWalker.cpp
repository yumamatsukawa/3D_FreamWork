#include "DemoWalker.h"
#include "../../Components/Common/DemoCameraController.h"
#include "../../Components/Common/DemoWalkerController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"

GameObject* CreateDemoWalker(Scene& scene, TerrainRenderer* terrain, DirectX::XMFLOAT2 startXZ) {
    // スタート地点: 地形の高さの少し上(地形が無ければ高さ100)
    const float groundHeight = terrain ? terrain->GetHeightAt(startXZ.x, startXZ.y) : 0.f;
    const DirectX::XMFLOAT3 spawn = { startXZ.x, groundHeight + 100.f, startXZ.y };

    GameObject* walker = scene.CreateObject("DemoWalker");
    walker->transform.position = spawn;
    walker->transform.scale = { 100.f, 100.f, 100.f };

    // 見た目(3Dメッシュの立方体。縁が青く光る)
    auto* meshRenderer = walker->AddComponent<MeshRenderer>(Mesh::CreateCube());
    meshRenderer->SetRimLight({ 0.3f, 0.7f, 1.0f }, 1.0f);

    // 当たり判定+物理演算(PhysX): 重力で落ちて、地形の上に乗る。ぶつかっても回転しない
    auto* rigidbody = walker->AddComponent<BoxRigidbodyComponent>(BodyType::Dynamic, walker->transform.scale, 100.f);
    rigidbody->SetUseGravity(true);
    rigidbody->SetFreezeRotation(true);

    // 3Dカメラ(右ドラッグで向き、Q/Eでズーム。歩く箱の少し上を、後ろから見る)
    // WalkerControllerより先に付けて、このフレームのカメラの向きを使えるようにする
    auto* camera = walker->AddComponent<DemoCameraController>(0.f, DirectX::XMConvertToRadians(-20.f), 650.f, 40.f);

    // 動き・入力操作(カメラの向きを基準にする)
    walker->AddComponent<DemoWalkerController>()->Setup(camera, spawn);
    return walker;
}
