#include "HomingDemo.h"
#include "DemoAgent.h"
#include "../../Components/Behavior/HomingDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Behavior/ChaseComponent.h"
#include "../../../../Engine/Behavior/AccelerateChaseComponent.h"
#include "../../../../Engine/Behavior/HomingComponent.h"

namespace {
    // 3つの出発点(左端に縦に並べる。平面の点。2Dは x,y、3Dは x,z)と、目標の最初の位置
    const DirectX::XMFLOAT2 starts2D[3] = { { -520.f, 0.f }, { -520.f, -100.f }, { -520.f, -200.f } };
    const DirectX::XMFLOAT2 starts3D[3] = { { -650.f, 200.f }, { -650.f, 20.f }, { -650.f, -160.f } };
    const DirectX::XMFLOAT2 firstTarget2D = { 300.f, -100.f };
    const DirectX::XMFLOAT2 firstTarget3D = { 350.f, 20.f };

    constexpr float agentSize2D = 40.f;
    constexpr float agentSize3D = 70.f;
}

GameObject* CreateHomingDemo(Scene& scene, DemoMode mode) {
    const bool is2D = (mode == DemoMode::Mode2D);
    const DirectX::XMFLOAT2* starts = is2D ? starts2D : starts3D;

    // 競技場(外の壁。ここでは動きを見るだけで、壁には当たらない)
    CreateDemoField(scene, mode, GetDemoField(mode));

    // 目標(マウスに付いてくる黄色い目印)
    GameObject* targetMarker = CreateDemoGoal(scene, mode, is2D ? firstTarget2D : firstTarget3D, 2, "TargetMarker");

    // 動く物3つ(色: 青・橙・緑)。Kinematic にして、お互いにすり抜けるようにする
    const int cells[3] = { 6, 1, 4 };
    const char* names[3] = { "ChaseAgent", "AccelerateAgent", "HomingAgent" };
    GameObject* agents[3];
    for (int i = 0; i < 3; i++) {
        DemoAgentDesc desc;
        desc.position = starts[i];
        desc.size = is2D ? agentSize2D : agentSize3D;
        desc.type = BodyType::Kinematic;
        desc.cell = cells[i];
        desc.name = names[i];
        agents[i] = CreateDemoAgent(scene, mode, desc);
    }

    // 動き(速さなどは、操作役が Setup で渡す)。目標は3つとも同じ目印
    const MovePlane plane = GetDemoPlane(mode);
    auto* chase = agents[0]->AddComponent<ChaseComponent>();
    chase->SetPlane(plane);
    chase->SetTarget(targetMarker);
    auto* accelerate = agents[1]->AddComponent<AccelerateChaseComponent>();
    accelerate->SetPlane(plane);
    accelerate->SetTarget(targetMarker);
    auto* homing = agents[2]->AddComponent<HomingComponent>();
    homing->SetPlane(plane);
    homing->SetTarget(targetMarker);
    homing->SetFaceMovement(true);   // 見た目の向きも進行方向に合わせる

    // 最初の進行方向は、出した時の向き(rotate。何も指定しないと上=2DはY+、3DはZ+)。目標は右にあるので、上へ飛び出してから右へ曲がっていく

    // 操作役(入り切り・設定・戻す・表示)
    GameObject* demo = scene.CreateObject("HomingDemo");
    demo->AddComponent<HomingDemoController>(mode)->Setup(agents[0], agents[1], agents[2], targetMarker);
    return demo;
}
