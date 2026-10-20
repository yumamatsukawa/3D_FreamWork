#include "HomingDemoController.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../Objects/Behavior/DemoAgent.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Behavior/ChaseComponent.h"
#include "../../../../Engine/Behavior/AccelerateChaseComponent.h"
#include "../../../../Engine/Behavior/HomingComponent.h"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {
    constexpr float markerMargin = 20.f;       // 目標の目印を競技場の縁から離す距離
    constexpr float markerLift = 20.f;         // 3D: マウスの線を当てる高さ(目印の中心の高さ)
    constexpr float trailInterval = 0.05f;     // 道の点を足す間隔(秒)
    constexpr float minTurnSpeed = 30.f;       // 曲がる速さの下限・上限(度/秒)
    constexpr float maxTurnSpeed = 720.f;
    constexpr float minAccelerationSteps = 1.f;    // 加速の下限・上限(1回の量の何倍か)
    constexpr float maxAccelerationSteps = 10.f;

    // 動きの基準の値(2Dの世界での値。3Dは unitScale 倍する)
    constexpr float chaseSpeed = 220.f;        // 青: 速さ
    constexpr float accelerateMaxSpeed = 350.f;   // 橙: 最高速度
    constexpr float accelerateDrag = 0.6f;     // 橙: 空気抵抗
    constexpr float homingSpeed = 220.f;       // 緑: 速さ
    constexpr float stopDistance = 30.f;       // 3つとも: 目標に近づいたら止まる距離

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 agentColors[3] = {
        { 0.5f, 0.75f, 1.f, 1.f },      // 青
        { 1.f, 0.7f, 0.4f, 1.f },       // 橙
        { 0.5f, 1.f, 0.55f, 1.f },      // 緑
    };
    const DirectX::XMFLOAT4 gray = { 0.75f, 0.75f, 0.75f, 1.f };
}

void HomingDemoController::Setup(GameObject* chaseObject, GameObject* accelerateObject, GameObject* homingObject, GameObject* targetObject) {
    agents[0] = chaseObject;
    agents[1] = accelerateObject;
    agents[2] = homingObject;
    targetMarker = targetObject;

    // 3つの動き
    auto* chase = agents[0]->GetComponent<ChaseComponent>();
    accelerate = agents[1]->GetComponent<AccelerateChaseComponent>();
    homing = agents[2]->GetComponent<HomingComponent>();
    motions[0] = chase;
    motions[1] = accelerate;
    motions[2] = homing;

    // 出発点・向きを覚える(Rで戻す)
    for (int i = 0; i < agentCount; i++) {
        startPositions[i] = agents[i]->transform.position;
        startRotates[i] = agents[i]->transform.rotate;
        previousPositions[i] = startPositions[i];
    }

    // 2Dと3Dで、世界の大きさが違うので、速さと加速も3Dは大きくする
    unitScale = (mode == DemoMode::Mode3D) ? 1.5f : 1.f;
    accelerationStep = 100.f * unitScale;
    acceleration = 500.f * unitScale;
    chase->SetSpeed(chaseSpeed * unitScale);
    accelerate->SetMaxSpeed(accelerateMaxSpeed * unitScale);
    accelerate->SetDrag(accelerateDrag);
    homing->SetSpeed(homingSpeed * unitScale);

    // 目標にこの距離まで近づいたら止まる(3つとも同じ。0に近いと、曲がる角度に限りがある緑は、目標の周りを回り続けてしまう)
    const float stop = stopDistance * unitScale;
    chase->SetStopDistance(stop);
    accelerate->SetStopDistance(stop);
    homing->SetStopDistance(stop);
    ApplySettings();
}

void HomingDemoController::ApplySettings() {
    accelerate->SetAcceleration(acceleration);
    homing->SetTurnSpeed(turnSpeed);
}

void HomingDemoController::Reset() {
    for (int i = 0; i < agentCount; i++) {
        agents[i]->transform.position = startPositions[i];
        agents[i]->transform.rotate = startRotates[i];
        previousPositions[i] = startPositions[i];
        directions[i] = { 0.f, 0.f, 0.f };
        trails[i].clear();

        // 勢い・進行方向を残さない(動きの OnActiveChanged が、状態をリセットする)
        agents[i]->SetActive(false);
        agents[i]->SetActive(true);
    }
}

void HomingDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1・2・3キー: 動きの入り切り(切ると、その場で止まる)
    const int keys[agentCount] = { KEY_1, KEY_2, KEY_3 };
    for (int i = 0; i < agentCount; i++) {
        if (Input::GetKeyDown(keys[i])) {
            active[i] = !active[i];
            motions[i]->SetEnabled(active[i]);
        }
    }

    // ↑↓: 緑の曲がる速さ、←→: 橙の加速
    if (Input::GetKeyDown(KEY_UP)) turnSpeed = (std::min)(turnSpeed + 30.f, maxTurnSpeed);
    if (Input::GetKeyDown(KEY_DOWN)) turnSpeed = (std::max)(turnSpeed - 30.f, minTurnSpeed);
    if (Input::GetKeyDown(KEY_RIGHT)) acceleration = (std::min)(acceleration + accelerationStep, accelerationStep * maxAccelerationSteps);
    if (Input::GetKeyDown(KEY_LEFT)) acceleration = (std::max)(acceleration - accelerationStep, accelerationStep * minAccelerationSteps);
    ApplySettings();

    // Rキー: 最初の位置へ戻す
    if (Input::GetKeyDown(KEY_R)) Reset();

    // 目標の目印は、マウスに付いてくる(3Dで空を向いている時は、前のまま)
    DirectX::XMFLOAT3 mouse;
    if (GetDemoMousePoint(mode, markerLift, mouse)) {
        PlaceDemoGoal(targetMarker, mode, ClampToDemoField(field, ConvertWorldToPlane(mode, mouse), markerMargin));
    }

    // 進んでいる向き(前のフレームからの位置の差)
    for (int i = 0; i < agentCount; i++) {
        DirectX::XMFLOAT3 moved = agents[i]->transform.position - previousPositions[i];
        moved.y = (mode == DemoMode::Mode3D) ? 0.f : moved.y;   // 3Dは水平だけ
        moved.z = (mode == DemoMode::Mode2D) ? 0.f : moved.z;   // 2DはXYだけ
        directions[i] = (dt > 0.f && Math::Length(moved) > 0.01f) ? Math::Normalize(moved) : DirectX::XMFLOAT3{ 0.f, 0.f, 0.f };
        previousPositions[i] = agents[i]->transform.position;
    }

    // 通った道: 一定の間隔で、今の位置を残す
    trailTimer -= dt;
    if (trailTimer <= 0.f) {
        trailTimer = trailInterval;
        for (int i = 0; i < agentCount; i++) {
            trails[i].push_back(agents[i]->transform.position);
            while (trails[i].size() > trailMax) trails[i].pop_front();
        }
    }
}

void HomingDemoController::Draw() {
    const bool is2D = (mode == DemoMode::Mode2D);
    const float arrowLength = is2D ? 45.f : 80.f;

    for (int i = 0; i < agentCount; i++) {
        // 通った道(古いほど小さく)
        for (size_t j = 0; j < trails[i].size(); j++) {
            const float size = 2.f + 4.f * (float)j / (float)trailMax;
            DrawDemoPoint(mode, trails[i][j], size, active[i] ? agentColors[i] : gray);
        }

        // 今進んでいる向き(白い線)
        if (active[i] && Math::Length(directions[i]) > 0.f) {
            const DirectX::XMFLOAT3 from = agents[i]->transform.position;
            DrawDemoLine(mode, from, from + directions[i] * arrowLength, white, 6);
        }
    }

    // 動く物の名前
    const wchar_t* names[agentCount] = { L"Chase", L"Accelerate", L"Homing" };
    for (int i = 0; i < agentCount; i++) {
        DrawDemoLabel(mode, agents[i]->transform.position, active[i] ? names[i] : L"(OFF)", is2D ? 34.f : 46.f);
    }

    // 状態
    wchar_t buf[160];
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"[1] 青 Chase: 一定の速さ %.0f でまっすぐ  %ls", chaseSpeed * unitScale, active[0] ? L"ON" : L"OFF");
    lines.push_back({ buf, agentColors[0] });
    swprintf_s(buf, L"[2] 橙 AccelerateChase: 加速 %.0f (←→)  %ls  勢いで行き過ぎる", acceleration, active[1] ? L"ON" : L"OFF");
    lines.push_back({ buf, agentColors[1] });
    swprintf_s(buf, L"[3] 緑 Homing: 曲がる速さ %.0f 度/秒 (↑↓)  %ls  大回りする", turnSpeed, active[2] ? L"ON" : L"OFF");
    lines.push_back({ buf, agentColors[2] });
    lines.push_back({ L"点: 通った道   白い線: 進んでいる向き   R 最初の位置へ戻す", white });
    DrawDemoStatus(lines, 740.f);
}
