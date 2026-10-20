#include "VisionDemo.h"
#include "DemoAgent.h"
#include "../../Components/Behavior/VisionDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Behavior/VisionComponent.h"

namespace {
    // 見張り役とターゲットの出発点(平面の点。2Dは x,y、3Dは x,z)
    const DirectX::XMFLOAT2 watcherPosition2D = { -440.f, -100.f }, targetStart2D = { 350.f, -100.f };
    const DirectX::XMFLOAT2 watcherPosition3D = { -650.f, 25.f }, targetStart3D = { 450.f, 25.f };

    // 途中の壁(1つ目は、見張り役とターゲットの間を横切る。2つ目は、ターゲットが隠れられる横長の壁)
    const DirectX::XMFLOAT2 wall1Center2D = { -100.f, -100.f }, wall1Size2D = { 40.f, 140.f };
    const DirectX::XMFLOAT2 wall2Center2D = { 250.f, -10.f }, wall2Size2D = { 140.f, 40.f };
    const DirectX::XMFLOAT2 wall1Center3D = { -200.f, 25.f }, wall1Size3D = { 60.f, 300.f };
    const DirectX::XMFLOAT2 wall2Center3D = { 250.f, -170.f }, wall2Size3D = { 260.f, 60.f };
    constexpr float wallHeight3D = 150.f;     // 3Dの壁の高さ(目を高くすると、壁の上から見える)

    constexpr float watcherSize2D = 50.f, targetSize2D = 40.f;
    constexpr float watcherSize3D = 80.f, targetSize3D = 70.f;
}

GameObject* CreateVisionDemo(Scene& scene, DemoMode mode, GameObject* camera) {
    const bool is2D = (mode == DemoMode::Mode2D);

    // 競技場(外の壁)と、途中の壁。壁にはタグ "Wall" が付く(VisionComponent の視線をさえぎる物)
    CreateDemoField(scene, mode, GetDemoField(mode));
    CreateDemoObstacle(scene, mode, is2D ? wall1Center2D : wall1Center3D, is2D ? wall1Size2D : wall1Size3D, wallHeight3D);
    CreateDemoObstacle(scene, mode, is2D ? wall2Center2D : wall2Center3D, is2D ? wall2Size2D : wall2Size3D, wallHeight3D);

    // 見張り役(動かない。正面は操作役が SetFacing で渡す)。視野の設定は、操作役が毎フレーム渡す
    DemoAgentDesc watcherDesc;
    watcherDesc.position = is2D ? watcherPosition2D : watcherPosition3D;
    watcherDesc.size = is2D ? watcherSize2D : watcherSize3D;
    watcherDesc.type = BodyType::Static;
    watcherDesc.cell = 6;
    watcherDesc.name = "Watcher";
    GameObject* watcher = CreateDemoAgent(scene, mode, watcherDesc);
    auto* vision = watcher->AddComponent<VisionComponent>();
    vision->SetPlane(GetDemoPlane(mode));
    vision->SetObstacleTag("Wall");

    // ターゲット(矢印キーで動かす。壁にぶつかる Dynamic)
    DemoAgentDesc targetDesc;
    targetDesc.position = is2D ? targetStart2D : targetStart3D;
    targetDesc.size = is2D ? targetSize2D : targetSize3D;
    targetDesc.cell = 4;
    targetDesc.name = "VisionTarget";
    GameObject* target = CreateDemoAgent(scene, mode, targetDesc);

    // 操作役(視野の設定・ターゲットの移動・表示)
    GameObject* demo = scene.CreateObject("VisionDemo");
    demo->AddComponent<VisionDemoController>(mode)->Setup(watcher, target, camera);
    return demo;
}
