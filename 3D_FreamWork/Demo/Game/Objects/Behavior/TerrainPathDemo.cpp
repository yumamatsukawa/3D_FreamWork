#include "TerrainPathDemo.h"
#include "DemoAgent.h"
#include "../../Components/Behavior/TerrainPathDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"
#include "../../../../Engine/Behavior/PathFollowComponent.h"

namespace {
    // 丘(幅・奥行き 4000。中心が原点)を、1マス50の格子でおおう
    const DirectX::XMFLOAT3 gridOrigin = { -2000.f, 0.f, -2000.f };
    const DirectX::XMFLOAT2 gridSize = { 4000.f, 4000.f };
    constexpr float cellSize = 50.f;
    constexpr float agentSize = 80.f;                 // 動く物の直径
    constexpr float defaultSlope = 20.f;              // 登れる坂の最初の角度

    // 出発点と、目的地の最初の位置(平面の点 x,z)
    const DirectX::XMFLOAT2 agentStart = { -1500.f, 800.f };
    const DirectX::XMFLOAT2 firstGoal = { 1200.f, -300.f };
}

NavGridComponent* CreateTerrainNavigation(Scene& scene, TerrainRenderer* terrain, float slopeDegrees) {
    NavGridSettings settings;
    settings.plane = MovePlane::XZ;
    settings.origin = gridOrigin;
    settings.size = gridSize;
    settings.cellSize = cellSize;
    settings.agentRadius = agentSize * 0.5f;
    settings.baseHeight = 0.f;                        // 丘が全体をおおうので、地形の無い所の高さは使わない
    settings.maxSlopeDegrees = slopeDegrees;

    GameObject* navigation = scene.CreateObject("Navigation");
    auto* nav = navigation->AddComponent<NavGridComponent>(settings);
    if (terrain) nav->SetTerrain(terrain);            // 焼く前に渡す。丘の高さ・傾きが各マスに入る
    return nav;
}

GameObject* CreateTerrainPathDemo(Scene& scene, GameObject* hills) {
    TerrainRenderer* terrain = hills ? hills->GetComponent<TerrainRenderer>() : nullptr;

    // クリックの位置を、地形に当てる Raycast で調べる(地面だけを対象にするタグ)
    if (hills) hills->SetTag("Terrain");

    // 経路探索の格子(丘の高さ・坂を取り込む)
    NavGridComponent* navigation = CreateTerrainNavigation(scene, terrain, defaultSlope);

    // 目的地の目印(丘の表面に置く)
    GameObject* goal = CreateDemoGoal(scene, DemoMode::Mode3D, firstGoal, 2, "Goal");
    if (terrain) goal->transform.position.y = terrain->GetHeightAt(firstGoal.x, firstGoal.y) + 20.f;

    // 緑の動く物(経路を探して、急な坂を避けて歩く)。丘の表面の少し上から落とす
    DemoAgentDesc agentDesc;
    agentDesc.position = agentStart;
    agentDesc.size = agentSize;
    agentDesc.cell = 4;
    agentDesc.name = "PathAgent";
    agentDesc.baseY = (terrain ? terrain->GetHeightAt(agentStart.x, agentStart.y) : 0.f) + 60.f;
    GameObject* agent = CreateDemoAgent(scene, DemoMode::Mode3D, agentDesc);
    auto* follow = agent->AddComponent<PathFollowComponent>();
    follow->SetPlane(MovePlane::XZ);
    follow->SetTarget(goal);
    follow->SetNavigation(navigation);

    // 操作役(目的地・登れる坂の角度・表示)
    GameObject* demo = scene.CreateObject("TerrainPathDemo");
    demo->AddComponent<TerrainPathDemoController>()->Setup(terrain, agent, goal, navigation, defaultSlope);
    return demo;
}
