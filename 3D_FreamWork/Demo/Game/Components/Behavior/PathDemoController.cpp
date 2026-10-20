#include "PathDemoController.h"
#include "../Common/DemoBodyReset.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../Objects/Behavior/DemoAgent.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Behavior/ChaseComponent.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"
#include "../../../../Engine/Behavior/PathFollowComponent.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {
    constexpr float goalMargin = 40.f;       // 目的地を競技場の縁から離す距離
    constexpr float goalLift = 20.f;         // 3D: マウスの線を当てる高さ(目印の中心の高さ)
    constexpr float stopDistance = 20.f;     // 目的地にここまで近づいたら止まる(2D。3Dは1.5倍)
    constexpr float minRepath = 0.2f, maxRepath = 2.0f;   // 探し直す間隔の範囲(秒)
    constexpr float stuckSpeed = 5.f;        // これより遅い時は、止まっているとみなす

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.55f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.45f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };
}

void PathDemoController::Init() {
    // 格子(通れないマス=赤い線)と経路(緑の線)を出す。全体の設定なので、Uninit とガイドの ResetSettings で消す
    NavGridComponent::SetDebugDrawEnabled(true);
    PathFollowComponent::SetDebugDrawEnabled(true);
}

void PathDemoController::Uninit() {
    NavGridComponent::SetDebugDrawEnabled(false);
    PathFollowComponent::SetDebugDrawEnabled(false);
}

void PathDemoController::Setup(GameObject* pathObject, GameObject* chaseObject, GameObject* goalObject, NavGridComponent* nav) {
    pathAgent = pathObject;
    chaseAgent = chaseObject;
    goal = goalObject;
    navigation = nav;
    follow = pathAgent->GetComponent<PathFollowComponent>();
    chase = chaseAgent->GetComponent<ChaseComponent>();
    pathStart = pathAgent->transform.position;
    chaseStart = chaseAgent->transform.position;

    // 3Dは世界が大きいので、速さの刻みも大きい
    if (mode == DemoMode::Mode3D) {
        speed = 270.f;
        speedStep = 45.f;
    }
    ApplySettings();
}

void PathDemoController::ApplySettings() {
    const float stop = (mode == DemoMode::Mode3D) ? stopDistance * 1.5f : stopDistance;
    follow->SetSpeed(speed);
    follow->SetStopDistance(stop);
    follow->SetRepathInterval(repathInterval);
    chase->SetSpeed(speed);
    chase->SetStopDistance(stop);
}

void PathDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 速さ(↑↓)・探し直す間隔(←→)
    if (Input::GetKeyDown(KEY_UP)) speed = (std::min)(speed + speedStep, speedStep * 8.f);
    if (Input::GetKeyDown(KEY_DOWN)) speed = (std::max)(speed - speedStep, speedStep);
    if (Input::GetKeyDown(KEY_RIGHT)) repathInterval = (std::min)(repathInterval + 0.2f, maxRepath);
    if (Input::GetKeyDown(KEY_LEFT)) repathInterval = (std::max)(repathInterval - 0.2f, minRepath);
    ApplySettings();

    // F1キー: 格子と経路の線を出す/消す
    if (Input::GetKeyDown(KEY_F1)) {
        gridVisible = !gridVisible;
        NavGridComponent::SetDebugDrawEnabled(gridVisible);
        PathFollowComponent::SetDebugDrawEnabled(gridVisible);
    }

    // Rキー: 出発点へ戻す
    if (Input::GetKeyDown(KEY_R)) {
        ResetDemoBody(pathAgent, pathStart);
        ResetDemoBody(chaseAgent, chaseStart);
    }

    // クリック: 目的地の目印を、その場所へ動かす(2つとも、この目印へ向かう)
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        DirectX::XMFLOAT3 mouse;
        if (GetDemoMousePoint(mode, goalLift, mouse)) {
            PlaceDemoGoal(goal, mode, ClampToDemoField(field, ConvertWorldToPlane(mode, mouse), goalMargin));
        }
    }
}

void PathDemoController::Draw() {
    const bool is2D = (mode == DemoMode::Mode2D);

    // 動く物の名前
    DrawDemoLabel(mode, pathAgent->transform.position, L"Path", is2D ? 44.f : 56.f);
    DrawDemoLabel(mode, chaseAgent->transform.position, L"Chase", is2D ? 44.f : 56.f);

    // 目的地までの距離(平面の上だけ)と、赤が壁に止められているか
    const DirectX::XMFLOAT2 goalPoint = ConvertWorldToPlane(mode, goal->transform.position);
    const float pathDistance = Math::Distance(ConvertWorldToPlane(mode, pathAgent->transform.position), goalPoint);
    const float chaseDistance = Math::Distance(ConvertWorldToPlane(mode, chaseAgent->transform.position), goalPoint);
    const RigidbodyComponent* chaseBody = chaseAgent->GetComponent<RigidbodyComponent>();
    const DirectX::XMFLOAT3 chaseVelocity = chaseBody ? chaseBody->GetVelocity() : DirectX::XMFLOAT3{ 0.f, 0.f, 0.f };
    const float chasePlaneSpeed = is2D ? Math::Length(DirectX::XMFLOAT2{ chaseVelocity.x, chaseVelocity.y }) : Math::Length(DirectX::XMFLOAT2{ chaseVelocity.x, chaseVelocity.z });
    const bool chaseBlocked = !chase->HasArrived() && chasePlaneSpeed < stuckSpeed;

    // 状態
    NavGrid& grid = navigation->GetGrid();
    wchar_t buf[200];
    std::vector<DemoStatusLine> lines;
    if (navigation->IsBaked()) {
        swprintf_s(buf, L"格子 NavGridComponent: %d x %d マス(1マス %.0f)  焼いた時間 %.2f ミリ秒",
            grid.GetCellCountX(), grid.GetCellCountZ(), grid.GetSettings().cellSize, navigation->GetLastBakeMilliseconds());
    }
    else {
        swprintf_s(buf, L"格子 NavGridComponent: 壁を読み込んでいます(IsBaked: いいえ)");
    }
    lines.push_back({ buf, cyan });
    swprintf_s(buf, L"緑 PathFollow: 経路 HasPath %ls(%d 点)  目的地まで %.0f", follow->HasPath() ? L"はい" : L"いいえ", (int)follow->GetPath().size(), pathDistance);
    lines.push_back({ buf, green });
    swprintf_s(buf, L"赤 Chase: 目的地まで %.0f  %ls", chaseDistance, chase->HasArrived() ? L"着いた" : (chaseBlocked ? L"壁に止められている" : L"まっすぐ向かっている"));
    lines.push_back({ buf, red });
    swprintf_s(buf, L"↑↓ 速さ %.0f   ←→ 探し直す間隔 SetRepathInterval %.1f 秒", speed, repathInterval);
    lines.push_back({ buf, white });
    lines.push_back({ L"クリック 目的地   F1 格子と経路の線   R 戻す", white });
    DrawDemoStatus(lines, 740.f);
}
