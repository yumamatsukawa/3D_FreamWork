#include "MoveDemo.h"
#include "DemoAgent.h"
#include "../../Components/Behavior/MoveDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Behavior/ChaseComponent.h"

namespace {
    // 青(マウスを追う)・橙(クリックの場所へ向かう)の出発点と、橙の最初の目的地(平面の点。2Dは x,y、3Dは x,z)
    const DirectX::XMFLOAT2 followStart2D = { -520.f, -60.f }, pointStart2D = { -520.f, -170.f }, firstDestination2D = { -380.f, -170.f };
    const DirectX::XMFLOAT2 followStart3D = { -650.f, 120.f }, pointStart3D = { -650.f, -150.f }, firstDestination3D = { -450.f, -150.f };

    // 途中の壁(まっすぐ進むだけの動きが、ぶつかって止まる所を見せる)
    const DirectX::XMFLOAT2 wallCenter2D = { -200.f, -100.f }, wallSize2D = { 40.f, 170.f };
    const DirectX::XMFLOAT2 wallCenter3D = { -250.f, -20.f }, wallSize3D = { 60.f, 330.f };

    constexpr float agentSize2D = 50.f;
    constexpr float agentSize3D = 80.f;
}

GameObject* CreateMoveDemo(Scene& scene, DemoMode mode) {
    const bool is2D = (mode == DemoMode::Mode2D);
    const DemoField field = GetDemoField(mode);

    // 競技場(外の壁)と、途中の壁
    CreateDemoField(scene, mode, field);
    CreateDemoObstacle(scene, mode, is2D ? wallCenter2D : wallCenter3D, is2D ? wallSize2D : wallSize3D);

    // 目印: 青いほうが追う「目標」(マウスに付いてくる)、橙のほうが向かう「目的地」(クリックで動く)
    GameObject* targetMarker = CreateDemoGoal(scene, mode, is2D ? followStart2D : followStart3D, 6, "TargetMarker");
    GameObject* destMarker = CreateDemoGoal(scene, mode, is2D ? firstDestination2D : firstDestination3D, 1, "DestinationMarker");

    // 青: 目標(マウスの目印)を追いかける。目標が動けば、追う先も変わる
    DemoAgentDesc followDesc;
    followDesc.position = is2D ? followStart2D : followStart3D;
    followDesc.size = is2D ? agentSize2D : agentSize3D;
    followDesc.cell = 6;
    followDesc.name = "FollowAgent";
    GameObject* followAgent = CreateDemoAgent(scene, mode, followDesc);
    auto* follow = followAgent->AddComponent<ChaseComponent>();
    follow->SetPlane(GetDemoPlane(mode));
    follow->SetTarget(targetMarker);

    // 橙: 決まった場所(クリックした場所)へ向かう。SetTargetPosition で渡す
    DemoAgentDesc pointDesc;
    pointDesc.position = is2D ? pointStart2D : pointStart3D;
    pointDesc.size = is2D ? agentSize2D : agentSize3D;
    pointDesc.cell = 1;
    pointDesc.name = "PointAgent";
    GameObject* pointAgent = CreateDemoAgent(scene, mode, pointDesc);
    auto* point = pointAgent->AddComponent<ChaseComponent>();
    point->SetPlane(GetDemoPlane(mode));
    point->SetTargetPosition(ConvertPlaneToWorld(mode, is2D ? firstDestination2D : firstDestination3D, 0.f));

    // 操作役(速さ・止まる距離・目印の移動・表示)
    GameObject* demo = scene.CreateObject("MoveDemo");
    demo->AddComponent<MoveDemoController>(mode)->Setup(followAgent, pointAgent, targetMarker, destMarker);
    return demo;
}
