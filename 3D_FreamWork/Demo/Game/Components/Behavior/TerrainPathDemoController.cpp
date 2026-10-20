#include "TerrainPathDemoController.h"
#include "../Common/DemoBodyReset.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../Objects/Behavior/DemoAgent.h"
#include "../../Objects/Behavior/TerrainPathDemo.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/Render3D/TerrainRenderer.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"
#include "../../../../Engine/Behavior/PathFollowComponent.h"
#include "../../../../Engine/Physics/Physics.h"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {
    constexpr float rayMax = 20000.f;        // マウスの線が届く限界
    constexpr float goalLift = 20.f;         // 目印を丘の表面から浮かせる高さ
    constexpr float stopDistance = 40.f;     // 目的地にここまで近づいたら止まる
    constexpr float speedStep = 50.f;        // 速さを1回で変える量
    const float slopeChoices[3] = { 15.f, 20.f, 30.f };   // 1・2・3キーで選べる、登れる坂の角度

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.55f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.45f, 0.4f, 1.f };
}

void TerrainPathDemoController::Init() {
    // 格子(通れないマス=赤い線)と経路(緑の線)を出す。全体の設定なので、Uninit とガイドの ResetSettings で消す
    NavGridComponent::SetDebugDrawEnabled(true);
    PathFollowComponent::SetDebugDrawEnabled(true);
}

void TerrainPathDemoController::Uninit() {
    NavGridComponent::SetDebugDrawEnabled(false);
    PathFollowComponent::SetDebugDrawEnabled(false);
}

void TerrainPathDemoController::Setup(TerrainRenderer* terrainRenderer, GameObject* agentObject, GameObject* goalObject, NavGridComponent* nav, float startSlope) {
    terrain = terrainRenderer;
    agent = agentObject;
    goal = goalObject;
    navigation = nav;
    slopeDegrees = startSlope;
    follow = agent->GetComponent<PathFollowComponent>();
    agentStart = agent->transform.position;
    follow->SetSpeed(speed);
    follow->SetStopDistance(stopDistance);
}

void TerrainPathDemoController::RebuildNavigation() {
    // 格子の設定は作る時に決まるので、角度を変えた新しい格子を作って、古い格子は Destroy する。歩く側には、新しい格子を渡し直す
    GameObject* oldNavigation = navigation->GetOwner();
    navigation = CreateTerrainNavigation(*GetOwner()->GetScene(), terrain, slopeDegrees);
    oldNavigation->Destroy();
    follow->SetNavigation(navigation);
}

bool TerrainPathDemoController::FindTerrainPoint(DirectX::XMFLOAT3& point) const {
    // マウスの位置から奥へ伸ばした線を、タグ "Terrain" の丘にだけ当てる(Input::GetMousePositionはY+=下なので、Yの符号を反転する)
    const DirectX::XMFLOAT2 mouse = Input::GetMousePosition();
    DirectX::XMFLOAT3 origin, direction;
    Image::ScreenPointToRay3D({ mouse.x, -mouse.y }, origin, direction);
    RaycastHit hit;
    if (!Physics::Raycast(origin, direction, rayMax, hit, "Terrain")) return false;
    point = hit.point;
    return true;
}

void TerrainPathDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1・2・3: 登れる坂の角度(変えた時だけ、格子を作り直す)
    const int keys[3] = { KEY_1, KEY_2, KEY_3 };
    for (int i = 0; i < 3; i++) {
        if (Input::GetKeyDown(keys[i]) && slopeDegrees != slopeChoices[i]) {
            slopeDegrees = slopeChoices[i];
            RebuildNavigation();
        }
    }

    // ↑↓: 速さ
    if (Input::GetKeyDown(KEY_UP)) speed = (std::min)(speed + speedStep, speedStep * 10.f);
    if (Input::GetKeyDown(KEY_DOWN)) speed = (std::max)(speed - speedStep, speedStep);
    follow->SetSpeed(speed);

    // F1: 格子と経路の線を出す/消す
    if (Input::GetKeyDown(KEY_F1)) {
        gridVisible = !gridVisible;
        NavGridComponent::SetDebugDrawEnabled(gridVisible);
        PathFollowComponent::SetDebugDrawEnabled(gridVisible);
    }

    // R: 出発点へ戻す
    if (Input::GetKeyDown(KEY_R)) ResetDemoBody(agent, agentStart);

    // クリック: 目的地の目印を、マウスの下の丘の表面に置く
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        DirectX::XMFLOAT3 point;
        if (FindTerrainPoint(point)) goal->transform.position = { point.x, point.y + goalLift, point.z };
    }
}

void TerrainPathDemoController::Draw() {
    DrawDemoLabel(DemoMode::Mode3D, agent->transform.position, L"Path", 56.f);

    // 通れないマスの数(格子が変わった時だけ数える)
    NavGrid& grid = navigation->GetGrid();
    if (navigation->IsBaked() && (countedGrid != navigation || countedVersion != grid.GetVersion())) {
        blockedCount = 0;
        for (int z = 0; z < grid.GetCellCountZ(); z++) {
            for (int x = 0; x < grid.GetCellCountX(); x++) {
                if (!grid.IsWalkable(x, z)) blockedCount++;
            }
        }
        countedGrid = navigation;
        countedVersion = grid.GetVersion();
    }

    // 緑の足元の地形の高さ(GetHeightAt)と、目的地までの距離(水平)
    const DirectX::XMFLOAT3 position = agent->transform.position;
    const float groundHeight = terrain ? terrain->GetHeightAt(position.x, position.z) : 0.f;
    const float distance = Math::Distance(DirectX::XMFLOAT2{ position.x, position.z }, DirectX::XMFLOAT2{ goal->transform.position.x, goal->transform.position.z });

    wchar_t buf[200];
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"[1][2][3] 登れる坂 maxSlopeDegrees: %.0f 度   足元の高さ GetHeightAt: %.0f", slopeDegrees, groundHeight);
    lines.push_back({ buf, cyan });
    if (navigation->IsBaked()) {
        swprintf_s(buf, L"格子 %d x %d マス  通れないマス(急な坂) %d 個  焼いた時間 %.2f ミリ秒",
            grid.GetCellCountX(), grid.GetCellCountZ(), blockedCount, navigation->GetLastBakeMilliseconds());
    }
    else {
        swprintf_s(buf, L"格子: 丘の高さを読み込んでいます(IsBaked: いいえ)");
    }
    lines.push_back({ buf, red });
    swprintf_s(buf, L"緑 PathFollow: 経路 HasPath %ls(%d 点)  目的地まで %.0f", follow->HasPath() ? L"はい" : L"いいえ", (int)follow->GetPath().size(), distance);
    lines.push_back({ buf, green });
    swprintf_s(buf, L"クリック 目的地   ↑↓ 速さ %.0f   F1 格子と経路の線   R 戻す", speed);
    lines.push_back({ buf, white });
    DrawDemoStatus(lines, 740.f);
}
