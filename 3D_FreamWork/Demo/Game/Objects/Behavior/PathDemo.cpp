#include "PathDemo.h"
#include "DemoAgent.h"
#include "../../Components/Behavior/PathDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Behavior/ChaseComponent.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"
#include "../../../../Engine/Behavior/PathFollowComponent.h"

namespace {
    // 通り道を作る2枚の壁(平面の点。2Dは x,y、3Dは x,z)。1枚目は下から伸びて上にすき間、2枚目は上から伸びて下にすき間
    const DirectX::XMFLOAT2 wallACenter2D = { -200.f, -155.f }, wallASize2D = { 40.f, 190.f };
    const DirectX::XMFLOAT2 wallBCenter2D = { 200.f, -45.f }, wallBSize2D = { 40.f, 190.f };
    const DirectX::XMFLOAT2 wallACenter3D = { -250.f, -100.f }, wallASize3D = { 60.f, 400.f };
    const DirectX::XMFLOAT2 wallBCenter3D = { 250.f, 125.f }, wallBSize3D = { 60.f, 450.f };

    // 出発点(緑・赤)と、目的地の最初の位置
    const DirectX::XMFLOAT2 pathStart2D = { -520.f, -100.f }, chaseStart2D = { -520.f, -200.f }, goal2D = { 520.f, -100.f };
    const DirectX::XMFLOAT2 pathStart3D = { -650.f, 0.f }, chaseStart3D = { -650.f, -200.f }, goal3D = { 600.f, 0.f };

    constexpr float agentSize2D = 50.f;
    constexpr float agentSize3D = 80.f;
    constexpr float cellSize2D = 25.f;      // 格子の1マス(動く物の半径くらい)
    constexpr float cellSize3D = 50.f;
}

GameObject* CreatePathDemo(Scene& scene, DemoMode mode) {
    const bool is2D = (mode == DemoMode::Mode2D);
    const DemoField field = GetDemoField(mode);
    const float agentSize = is2D ? agentSize2D : agentSize3D;

    // 競技場と、通り道の壁(全部タグ "Wall")
    CreateDemoField(scene, mode, field);
    CreateDemoObstacle(scene, mode, is2D ? wallACenter2D : wallACenter3D, is2D ? wallASize2D : wallASize3D);
    CreateDemoObstacle(scene, mode, is2D ? wallBCenter2D : wallBCenter3D, is2D ? wallBSize2D : wallBSize3D);

    // 経路探索の格子(壁を置いた後に作る。agentRadius は動く物の半径に少し足した値)
    NavGridComponent* navigation = CreateDemoNavigation(scene, mode, field, is2D ? cellSize2D : cellSize3D, agentSize * 0.5f + (is2D ? 1.f : 0.f));

    // 目的地の目印(クリックで動く)
    GameObject* goal = CreateDemoGoal(scene, mode, is2D ? goal2D : goal3D, 2, "Goal");

    // 緑: 格子の上で経路を探して、壁を避けて向かう
    DemoAgentDesc pathDesc;
    pathDesc.position = is2D ? pathStart2D : pathStart3D;
    pathDesc.size = agentSize;
    pathDesc.cell = 4;
    pathDesc.name = "PathAgent";
    GameObject* pathAgent = CreateDemoAgent(scene, mode, pathDesc);
    auto* follow = pathAgent->AddComponent<PathFollowComponent>();
    follow->SetPlane(GetDemoPlane(mode));
    follow->SetTarget(goal);
    follow->SetNavigation(navigation);

    // 赤: 経路を探さず、まっすぐ向かう(壁にぶつかると、そこで止まる)
    DemoAgentDesc chaseDesc;
    chaseDesc.position = is2D ? chaseStart2D : chaseStart3D;
    chaseDesc.size = agentSize;
    chaseDesc.cell = 0;
    chaseDesc.name = "ChaseAgent";
    GameObject* chaseAgent = CreateDemoAgent(scene, mode, chaseDesc);
    auto* chase = chaseAgent->AddComponent<ChaseComponent>();
    chase->SetPlane(GetDemoPlane(mode));
    chase->SetTarget(goal);

    // 操作役(目印の移動・速さ・探し直す間隔・表示)
    GameObject* demo = scene.CreateObject("PathDemo");
    demo->AddComponent<PathDemoController>(mode)->Setup(pathAgent, chaseAgent, goal, navigation);
    return demo;
}
