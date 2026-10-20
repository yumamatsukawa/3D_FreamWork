#include "MoveDemoController.h"
#include "../Common/DemoBodyReset.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../Objects/Behavior/DemoAgent.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Behavior/ChaseComponent.h"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {
    constexpr float markerMargin = 20.f;     // 目印を競技場の縁から離す距離
    constexpr float markerLift = 20.f;       // 3D: マウスの線を当てる高さ(目印の中心の高さ)
    constexpr float maxSpeedSteps = 8.f;     // 速さの上限(1回の量の何倍か)
    constexpr float maxStopSteps = 10.f;     // 止まる距離の上限

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 blue = { 0.5f, 0.75f, 1.f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.7f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
}

void MoveDemoController::Setup(GameObject* followObject, GameObject* pointObject, GameObject* targetObject, GameObject* destObject) {
    followAgent = followObject;
    pointAgent = pointObject;
    targetMarker = targetObject;
    destMarker = destObject;
    follow = followAgent->GetComponent<ChaseComponent>();
    point = pointAgent->GetComponent<ChaseComponent>();
    followStart = followAgent->transform.position;
    pointStart = pointAgent->transform.position;

    // 2Dと3Dで、世界の大きさが違うので、速さと止まる距離も違う
    if (mode == DemoMode::Mode3D) {
        speed = 300.f;
        stopDistance = 90.f;
        speedStep = 75.f;
        stopStep = 30.f;
    }
    ApplySettings();
}

void MoveDemoController::ApplySettings() {
    for (ChaseComponent* chase : { follow, point }) {
        if (!chase) continue;
        chase->SetSpeed(speed);
        chase->SetStopDistance(stopDistance);
    }
}

void MoveDemoController::Reset() {
    ResetDemoBody(followAgent, followStart);
    ResetDemoBody(pointAgent, pointStart);
}

void MoveDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 速さ(↑↓)・止まる距離(←→)
    if (Input::GetKeyDown(KEY_UP)) speed = (std::min)(speed + speedStep, speedStep * maxSpeedSteps);
    if (Input::GetKeyDown(KEY_DOWN)) speed = (std::max)(speed - speedStep, speedStep);
    if (Input::GetKeyDown(KEY_RIGHT)) stopDistance = (std::min)(stopDistance + stopStep, stopStep * maxStopSteps);
    if (Input::GetKeyDown(KEY_LEFT)) stopDistance = (std::max)(stopDistance - stopStep, 0.f);
    ApplySettings();

    // Rキー: 出発点へ戻す
    if (Input::GetKeyDown(KEY_R)) Reset();

    // マウスの下の位置(3Dで空を向いている時は、前のまま)
    DirectX::XMFLOAT3 mouse;
    if (!GetDemoMousePoint(mode, markerLift, mouse)) return;
    const DirectX::XMFLOAT2 mousePoint = ClampToDemoField(field, ConvertWorldToPlane(mode, mouse), markerMargin);

    // 目標の目印はマウスに付いてくる(青い動く物が、これを追う)
    PlaceDemoGoal(targetMarker, mode, mousePoint);

    // クリック: 橙の目的地を、その場所に変える
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        PlaceDemoGoal(destMarker, mode, mousePoint);
        point->SetTargetPosition(ConvertPlaneToWorld(mode, mousePoint, 0.f));
    }
}

void MoveDemoController::Draw() {
    // 目標・目的地の周りの円: 止まる距離(この円に入ると止まる)
    if (stopDistance > 1.f) {
        DrawDemoRing(mode, targetMarker->transform.position, stopDistance, blue);
        DrawDemoRing(mode, destMarker->transform.position, stopDistance, orange);
    }

    // 動く物の名前
    DrawDemoLabel(mode, followAgent->transform.position, L"A Chase", 46.f);
    DrawDemoLabel(mode, pointAgent->transform.position, L"B Move", 46.f);

    // 目標までの距離(平面の上だけ)と、着いたか
    const DirectX::XMFLOAT2 followPos = ConvertWorldToPlane(mode, followAgent->transform.position);
    const DirectX::XMFLOAT2 pointPos = ConvertWorldToPlane(mode, pointAgent->transform.position);
    const float followDistance = Math::Distance(followPos, ConvertWorldToPlane(mode, targetMarker->transform.position));
    const float pointDistance = Math::Distance(pointPos, ConvertWorldToPlane(mode, destMarker->transform.position));

    wchar_t buf[160];
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"速さ SetSpeed: %.0f   止まる距離 SetStopDistance: %.0f", speed, stopDistance);
    lines.push_back({ buf, white });
    swprintf_s(buf, L"青A SetTarget(マウス)  距離 %.0f  %ls", followDistance, follow->HasArrived() ? L"着いた" : L"追跡中");
    lines.push_back({ buf, blue });
    swprintf_s(buf, L"橙B SetTargetPosition(クリックの場所)  距離 %.0f  %ls", pointDistance, point->HasArrived() ? L"着いた" : L"移動中");
    lines.push_back({ buf, orange });
    lines.push_back({ L"↑↓ 速さ   ←→ 止まる距離   クリック Bの目的地   R 戻す", green });
    DrawDemoStatus(lines, 740.f);
}
