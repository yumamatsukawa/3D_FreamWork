#include "MovingWallDemoController.h"
#include "DoorController.h"
#include "../Common/DemoBodyReset.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../Objects/Behavior/DemoAgent.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Behavior/NavGridComponent.h"
#include "../../../../Engine/Behavior/PathFollowComponent.h"
#include <cstdio>

namespace {
    constexpr float autoRebakeInterval = 0.25f;   // 自動で焼き直す間隔(秒)
    constexpr float clickMargin = 40.f;           // 目的地・柵を競技場の縁から離す距離
    constexpr float clickLift = 20.f;             // 3D: マウスの線を当てる高さ
    constexpr float doorGuard2D = 110.f;          // 緑がドアの通り道からこの距離にいる間は、ドアを閉めない
    constexpr float doorGuard3D = 180.f;
    constexpr float refuseTime = 2.f;             // 「まだ閉められません」を出す時間(秒)

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.7f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.55f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.45f, 0.4f, 1.f };
}

void MovingWallDemoController::Init() {
    // 格子(通れないマス=赤い線)と経路(緑の線)を出す。全体の設定なので、Uninit とガイドの ResetSettings で消す
    NavGridComponent::SetDebugDrawEnabled(true);
    PathFollowComponent::SetDebugDrawEnabled(true);
}

void MovingWallDemoController::Uninit() {
    NavGridComponent::SetDebugDrawEnabled(false);
    PathFollowComponent::SetDebugDrawEnabled(false);
}

void MovingWallDemoController::Setup(GameObject* agentObject, GameObject* goalObject, DoorController* doorController, NavGridComponent* nav) {
    agent = agentObject;
    goal = goalObject;
    door = doorController;
    navigation = nav;
    follow = agent->GetComponent<PathFollowComponent>();
    agentStart = agent->transform.position;

    // 2Dと3Dで、世界の大きさが違うので、速さ・止まる距離・柵の大きさも違う
    const bool is2D = (mode == DemoMode::Mode2D);
    follow->SetSpeed(is2D ? 180.f : 270.f);
    follow->SetStopDistance(is2D ? 20.f : 30.f);
    fenceSize = is2D ? DirectX::XMFLOAT3{ 100.f, 100.f, 100.f } : DirectX::XMFLOAT3{ 180.f, 100.f, 180.f };

    // ドアが動いたら自動で焼き直す(動く壁を、格子が追いかける)
    navigation->SetAutoRebakeInterval(autoRebakeInterval);
}

void MovingWallDemoController::ApplyFence(const DirectX::XMFLOAT2& center, bool blocked) {
    navigation->GetGrid().SetBlockedBox(ConvertPlaneToWorld(mode, center, 0.f), fenceSize, blocked);
}

void MovingWallDemoController::AddFence(const DirectX::XMFLOAT2& center) {
    // 上限を超える時は、一番古い柵を外す。柵どうしが重なっていると、外した時に残りのマスまで通れるようになるので、残りを登録し直す
    if (fences.size() >= maxFences) {
        ApplyFence(fences.front(), false);
        fences.erase(fences.begin());
        for (const DirectX::XMFLOAT2& fence : fences) ApplyFence(fence, true);
    }
    fences.push_back(center);
    ApplyFence(center, true);
}

void MovingWallDemoController::ClearFences() {
    for (const DirectX::XMFLOAT2& fence : fences) ApplyFence(fence, false);
    fences.clear();
}

void MovingWallDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1: ドアの開け閉め(緑がドアの通り道にいる間は、閉めない。挟まって動けなくなるのを防ぐ)
    if (Input::GetKeyDown(KEY_1)) {
        const float guard = (mode == DemoMode::Mode2D) ? doorGuard2D : doorGuard3D;
        const float distance = Math::Distance(ConvertWorldToPlane(mode, agent->transform.position), ConvertWorldToPlane(mode, door->GetClosedPosition()));
        if (!door->IsOpen() || distance > guard) door->SetOpen(!door->IsOpen());
        else refuseTimer = refuseTime;
    }

    refuseTimer -= dt;

    // 2: クリックの意味(目的地 / 柵)
    if (Input::GetKeyDown(KEY_2)) placeFence = !placeFence;

    // 3: 自動の焼き直しの入り切り。4: 今すぐ焼き直す(次のフレームで焼かれる)
    if (Input::GetKeyDown(KEY_3)) {
        autoRebake = !autoRebake;
        navigation->SetAutoRebakeInterval(autoRebake ? autoRebakeInterval : 0.f);
    }
    if (Input::GetKeyDown(KEY_4)) navigation->RequestRebake();

    // C: 柵を全部消す
    if (Input::GetKeyDown(KEY_C)) ClearFences();

    // F1: 格子と経路の線を出す/消す
    if (Input::GetKeyDown(KEY_F1)) {
        gridVisible = !gridVisible;
        NavGridComponent::SetDebugDrawEnabled(gridVisible);
        PathFollowComponent::SetDebugDrawEnabled(gridVisible);
    }

    // R: 緑を出発点へ戻す
    if (Input::GetKeyDown(KEY_R)) ResetDemoBody(agent, agentStart);

    // クリック: 目的地の目印を動かす/柵を置く
    if (Input::GetKeyDown(MOUSE_LEFT)) {
        DirectX::XMFLOAT3 mouse;
        if (GetDemoMousePoint(mode, clickLift, mouse)) {
            const DirectX::XMFLOAT2 point = ClampToDemoField(field, ConvertWorldToPlane(mode, mouse), clickMargin);
            if (placeFence) AddFence(point);
            else PlaceDemoGoal(goal, mode, point);
        }
    }
}

void MovingWallDemoController::Draw() {
    const bool is2D = (mode == DemoMode::Mode2D);

    // 柵(物理の無い障害物。格子にだけ登録してあるので、枠で見せる)
    const DirectX::XMFLOAT2 fenceRect = is2D ? DirectX::XMFLOAT2{ fenceSize.x, fenceSize.y } : DirectX::XMFLOAT2{ fenceSize.x, fenceSize.z };
    for (const DirectX::XMFLOAT2& fence : fences) {
        DrawDemoRect(mode, ConvertPlaneToWorld(mode, fence, 0.f), fenceRect, orange);
    }

    // ドアと緑の名前
    DrawDemoLabel(mode, door->GetOwner()->transform.position, L"ドア", is2D ? 70.f : 130.f);
    DrawDemoLabel(mode, agent->transform.position, L"Path", is2D ? 44.f : 56.f);

    // 目的地までの距離(平面の上だけ)
    const float distance = Math::Distance(ConvertWorldToPlane(mode, agent->transform.position), ConvertWorldToPlane(mode, goal->transform.position));

    // 状態
    const wchar_t* doorText = door->IsOpen() ? (door->IsMoving() ? L"開いていく" : L"開いている") : (door->IsMoving() ? L"閉じていく" : L"閉じている");
    wchar_t buf[200];
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"[1] ドア(動く壁): %ls   [3] 自動の焼き直し: %ls   [4] 今すぐ焼く", doorText, autoRebake ? L"ON(0.25秒ごと)" : L"OFF");
    lines.push_back({ buf, orange });
    if (autoRebake) {
        swprintf_s(buf, L"格子の version %d   最後に焼いた時間 %.2f ミリ秒", navigation->GetGrid().GetVersion(), navigation->GetLastBakeMilliseconds());
    }
    else {
        swprintf_s(buf, L"格子の version %d   焼き直しOFF: 4 を押すまで、格子は古いまま", navigation->GetGrid().GetVersion());
    }
    lines.push_back({ buf, cyan });
    swprintf_s(buf, L"緑 PathFollow: 経路 HasPath %ls(%d 点)  目的地まで %.0f", follow->HasPath() ? L"はい" : L"いいえ", (int)follow->GetPath().size(), distance);
    lines.push_back({ buf, green });
    swprintf_s(buf, L"[2] クリック: %ls   柵 SetBlockedBox: %d / %d 個   C 消す", placeFence ? L"柵を置く" : L"目的地を置く", (int)fences.size(), (int)maxFences);
    lines.push_back({ buf, white });
    lines.push_back({ L"F1 格子と経路の線   R 戻す", white });
    if (refuseTimer > 0.f) lines.push_back({ L"緑がドアの通り道にいるので、まだ閉められません", red });
    DrawDemoStatus(lines, 740.f);
}
