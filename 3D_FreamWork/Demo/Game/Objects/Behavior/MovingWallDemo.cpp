#include "MovingWallDemo.h"
#include "DemoAgent.h"
#include "../../Components/Behavior/DoorController.h"
#include "../../Components/Behavior/MovingWallDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"
#include "../../../../Engine/Behavior/PathFollowComponent.h"

namespace {
    // 真ん中の壁(x=0)の固定の部分: 下のドアと、上の広いすき間の間の壁(平面の点。2Dは x,y、3Dは x,z)
    const DirectX::XMFLOAT2 middleCenter2D = { 0.f, -95.f }, middleSize2D = { 40.f, 110.f };      // y: -150〜-40
    const DirectX::XMFLOAT2 middleCenter3D = { 0.f, 0.f }, middleSize3D = { 60.f, 200.f };       // z: -100〜100

    // ドア: 閉じた位置と大きさ。開いた位置は、固定の壁の中へ(2Dは上へ1枚分、3Dは上へ持ち上げる)
    const DirectX::XMFLOAT2 doorCenter2D = { 0.f, -200.f }, doorSize2D = { 40.f, 100.f };        // y: -250〜-150
    const DirectX::XMFLOAT2 doorCenter3D = { 0.f, -200.f }, doorSize3D = { 60.f, 200.f };        // z: -300〜-100
    constexpr float doorHeight3D = 200.f;
    constexpr float doorOpenShift2D = 100.f;     // 2D: 開く時に上へ動く距離
    constexpr float doorOpenShift3D = 260.f;     // 3D: 開く時に持ち上げる高さ
    constexpr float doorSpeed2D = 160.f;
    constexpr float doorSpeed3D = 350.f;

    // 出発点と、目的地の最初の位置(ドアの下の通り道の高さに合わせる)
    const DirectX::XMFLOAT2 start2D = { -520.f, -200.f }, goal2D = { 520.f, -200.f };
    const DirectX::XMFLOAT2 start3D = { -650.f, -200.f }, goal3D = { 600.f, -200.f };

    constexpr float agentSize2D = 44.f;      // 2Dは壁のすき間(90と100)を通れるよう、少し小さい動く物と細かい格子にする
    constexpr float agentSize3D = 80.f;
    constexpr float cellSize2D = 20.f;
    constexpr float cellSize3D = 50.f;
}

GameObject* CreateMovingWallDemo(Scene& scene, DemoMode mode) {
    const bool is2D = (mode == DemoMode::Mode2D);
    const DemoField field = GetDemoField(mode);
    const float agentSize = is2D ? agentSize2D : agentSize3D;

    // 競技場と、真ん中の壁の固定の部分(タグ "Wall")
    CreateDemoField(scene, mode, field);
    CreateDemoObstacle(scene, mode, is2D ? middleCenter2D : middleCenter3D, is2D ? middleSize2D : middleSize3D);

    // ドア(動く壁。Kinematic の箱で、タグ "Wall")。見た目は固定の壁と少し違う色にする
    GameObject* door = scene.CreateObject("Door", "Wall");
    const DirectX::XMFLOAT3 closed = is2D ? DirectX::XMFLOAT3{ doorCenter2D.x, doorCenter2D.y, 1.f }
                                          : DirectX::XMFLOAT3{ doorCenter3D.x, demoGroundTop + doorHeight3D * 0.5f, doorCenter3D.y };
    DirectX::XMFLOAT3 opened = closed;
    if (is2D) {
        door->transform.scale = { doorSize2D.x, doorSize2D.y, 1.f };
        auto* sprite = door->AddComponent<SpriteRenderer>(L"Assets/button.png");
        sprite->SetMode(SpriteMode::World2D);
        sprite->SetColor(1.f, 0.7f, 0.4f, 1.f);
        door->AddComponent<BoxRigidbodyComponent>(BodyType::Kinematic, DirectX::XMFLOAT3{ doorSize2D.x, doorSize2D.y, 200.f });
        opened.y += doorOpenShift2D;
    }
    else {
        door->transform.scale = { doorSize3D.x, doorHeight3D, doorSize3D.y };
        door->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(1.f, 0.65f, 0.3f, 1.f);
        door->AddComponent<BoxRigidbodyComponent>(BodyType::Kinematic, DirectX::XMFLOAT3{ doorSize3D.x, doorHeight3D, doorSize3D.y });
        opened.y += doorOpenShift3D;
    }
    auto* doorController = door->AddComponent<DoorController>();
    doorController->Setup(closed, opened, is2D ? doorSpeed2D : doorSpeed3D);

    // 経路探索の格子(壁とドアを置いた後に作る)
    NavGridComponent* navigation = CreateDemoNavigation(scene, mode, field, is2D ? cellSize2D : cellSize3D, agentSize * 0.5f);

    // 目的地の目印と、緑の動く物(経路を探して歩く)
    GameObject* goal = CreateDemoGoal(scene, mode, is2D ? goal2D : goal3D, 2, "Goal");
    DemoAgentDesc agentDesc;
    agentDesc.position = is2D ? start2D : start3D;
    agentDesc.size = agentSize;
    agentDesc.cell = 4;
    agentDesc.name = "PathAgent";
    GameObject* agent = CreateDemoAgent(scene, mode, agentDesc);
    auto* follow = agent->AddComponent<PathFollowComponent>();
    follow->SetPlane(GetDemoPlane(mode));
    follow->SetTarget(goal);
    follow->SetNavigation(navigation);

    // 操作役(ドアの開け閉め・焼き直しの設定・柵・表示)
    GameObject* demo = scene.CreateObject("MovingWallDemo");
    demo->AddComponent<MovingWallDemoController>(mode)->Setup(agent, goal, doorController, navigation);
    return demo;
}
