#include "DemoAgent.h"
#include "../Common/DemoBody.h"
#include "../Common/DemoWall.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"

namespace {
    constexpr float goalSize = 30.f;           // 目印の大きさ
    constexpr float goalLift3D = 20.f;         // 3D: 目印の中心の、地面の上面からの高さ
    constexpr float goalZ2D = -1.f;            // 2D: 目印のz(動く物の z=0 より手前に描く)
    constexpr float fieldThickness2D = 30.f;   // 競技場の壁の厚み
    constexpr float fieldThickness3D = 60.f;
}

DirectX::XMFLOAT4 GetDemoCellColor(int cell) {
    static const DirectX::XMFLOAT4 colors[9] = {
        { 0.75f, 0.15f, 0.30f, 1.f },   // 0 えんじ
        { 0.90f, 0.42f, 0.25f, 1.f },   // 1 オレンジ
        { 0.95f, 0.70f, 0.20f, 1.f },   // 2 黄
        { 0.60f, 0.90f, 0.30f, 1.f },   // 3 黄緑
        { 0.35f, 0.75f, 0.35f, 1.f },   // 4 緑
        { 0.00f, 0.40f, 0.33f, 1.f },   // 5 深緑
        { 0.20f, 0.50f, 0.90f, 1.f },   // 6 青
        { 0.40f, 0.40f, 0.70f, 1.f },   // 7 青紫
        { 0.25f, 0.25f, 0.50f, 1.f },   // 8 紺
    };
    return colors[(cell < 0 || cell > 8) ? 6 : cell];
}

GameObject* CreateDemoAgent(Scene& scene, DemoMode mode, const DemoAgentDesc& desc) {
    // 見た目と剛体は、Physicsのデモと同じ部品(CreateDemoBody2D / 3D)で作る
    DemoBodyDesc body;
    body.shape = DemoShape::Round;
    body.type = desc.type;
    body.size = desc.size;
    body.cell = desc.cell;
    body.color = GetDemoCellColor(desc.cell);
    body.name = desc.name;
    body.tag = desc.tag;
    body.position = ConvertPlaneToWorld(mode, desc.position, desc.size * 0.5f);
    if (mode == DemoMode::Mode3D) body.position.y = desc.baseY + desc.size * 0.5f;   // 3D: 地面(地形)の上に乗せる

    if (mode == DemoMode::Mode2D) return CreateDemoBody2D(scene, body);

    // 3D: 動く球は転がらないよう、回転を固定する(押された時に向きが変わらない)
    GameObject* agent = CreateDemoBody3D(scene, body);
    if (desc.type == BodyType::Dynamic) agent->GetComponent<RigidbodyComponent>()->SetFreezeRotation(true);
    return agent;
}

void SetDemoAgentCell(GameObject* agent, DemoMode mode, int cell) {
    if (!agent) return;
    if (mode == DemoMode::Mode2D) {
        if (SpriteRenderer* sprite = agent->GetComponent<SpriteRenderer>()) sprite->SetSpriteIndex(cell);
        return;
    }
    if (MeshRenderer* mesh = agent->GetComponent<MeshRenderer>()) {
        const DirectX::XMFLOAT4 color = GetDemoCellColor(cell);
        mesh->SetColor(color.x, color.y, color.z, color.w);
    }
}

GameObject* CreateDemoGoal(Scene& scene, DemoMode mode, DirectX::XMFLOAT2 position, int cell, const std::string& name) {
    GameObject* goal = scene.CreateObject(name);
    goal->transform.scale = { goalSize, goalSize, 1.f };

    if (mode == DemoMode::Mode2D) {
        // 2D: 丸い絵(box.png の1コマ)。2Dモードのスプライト
        SpriteRenderer* sprite = goal->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3);
        sprite->SetSpriteIndex(cell);
        sprite->SetMode(SpriteMode::World2D);
    }
    else {
        // 3D: 小さな球
        goal->transform.scale = { goalSize, goalSize, goalSize };
        const DirectX::XMFLOAT4 color = GetDemoCellColor(cell);
        goal->AddComponent<MeshRenderer>(Mesh::CreateSphere())->SetColor(color.x, color.y, color.z, color.w);
    }
    PlaceDemoGoal(goal, mode, position);
    return goal;
}

void PlaceDemoGoal(GameObject* goal, DemoMode mode, DirectX::XMFLOAT2 position) {
    if (!goal) return;
    goal->transform.position = ConvertPlaneToWorld(mode, position, goalLift3D);
    if (mode == DemoMode::Mode2D) goal->transform.position.z = goalZ2D;
}

GameObject* CreateDemoObstacle(Scene& scene, DemoMode mode, DirectX::XMFLOAT2 center, DirectX::XMFLOAT2 size, float height) {
    GameObject* wall = nullptr;
    if (mode == DemoMode::Mode2D) {
        wall = CreateDemoWall2D(scene, center, size);
    }
    else {
        // 3D: 地面の上面から height の高さの箱
        wall = CreateDemoWall3D(scene, { center.x, demoGroundTop + height * 0.5f, center.y }, { size.x, height, size.y });
    }
    wall->SetTag("Wall");
    return wall;
}

void CreateDemoField(Scene& scene, DemoMode mode, const DemoField& field) {
    const float thickness = (mode == DemoMode::Mode2D) ? fieldThickness2D : fieldThickness3D;
    const float width = field.max.x - field.min.x;
    const float depth = field.max.y - field.min.y;
    const float centerX = (field.min.x + field.max.x) * 0.5f;
    const float centerY = (field.min.y + field.max.y) * 0.5f;

    // 下・上・左・右の順に置く(上下は左右の壁の厚みの分まで伸ばして、角にすき間を作らない)
    CreateDemoObstacle(scene, mode, { centerX, field.min.y - thickness * 0.5f }, { width + thickness * 2.f, thickness });
    CreateDemoObstacle(scene, mode, { centerX, field.max.y + thickness * 0.5f }, { width + thickness * 2.f, thickness });
    CreateDemoObstacle(scene, mode, { field.min.x - thickness * 0.5f, centerY }, { thickness, depth });
    CreateDemoObstacle(scene, mode, { field.max.x + thickness * 0.5f, centerY }, { thickness, depth });
}

NavGridComponent* CreateDemoNavigation(Scene& scene, DemoMode mode, const DemoField& field, float cellSize, float agentRadius) {
    // 格子の範囲は競技場の内側。2Dは XY の面(z=0)、3Dは XZ の面(地面の高さは demoGroundTop)
    NavGridSettings settings;
    settings.plane = GetDemoPlane(mode);
    settings.origin = (mode == DemoMode::Mode2D) ? DirectX::XMFLOAT3{ field.min.x, field.min.y, 0.f } : DirectX::XMFLOAT3{ field.min.x, 0.f, field.min.y };
    settings.size = { field.max.x - field.min.x, field.max.y - field.min.y };
    settings.cellSize = cellSize;
    settings.agentRadius = agentRadius;
    settings.baseHeight = demoGroundTop;

    GameObject* navigation = scene.CreateObject("Navigation");
    return navigation->AddComponent<NavGridComponent>(settings);
}
