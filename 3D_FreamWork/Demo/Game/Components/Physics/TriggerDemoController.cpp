#include "TriggerDemoController.h"
#include "TouchCounterController.h"
#include "../Common/DemoCameraController.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"
#include <cmath>
#include <cstdio>

namespace {
    constexpr float moveSpeed = 180.f;        // 球の速さ(ゆっくり。速くぶつけると跳ね返って、ぶつかった瞬間(Enter)が何回も呼ばれてしまう)
    constexpr float zoneAlphaOut = 0.6f;     // トリガーの濃さ(球が外にいる時)
    constexpr float zoneAlphaIn = 0.95f;      // トリガーの濃さ(球が中にいる時)

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 blue = { 0.6f, 0.8f, 1.f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };
}

void TriggerDemoController::Setup(GameObject* moverObject, GameObject* zoneObject, GameObject* solidObject, GameObject* cameraObject) {
    mover = moverObject;
    moverBody = mover->GetComponent<RigidbodyComponent>();
    zone = zoneObject;
    solid = solidObject;
    zoneCounter = zone->GetComponent<TouchCounterController>();
    solidCounter = solid->GetComponent<TouchCounterController>();
    camera = cameraObject;
}

void TriggerDemoController::MoveMover() {
    if (!moverBody) return;

    // 入力の向き(右=x+、上/奥=y+)
    DirectX::XMFLOAT2 input = { 0.f, 0.f };
    if (Input::GetKeyPress(KEY_D) || Input::GetKeyPress(KEY_RIGHT)) input.x += 1.f;
    if (Input::GetKeyPress(KEY_A) || Input::GetKeyPress(KEY_LEFT)) input.x -= 1.f;
    if (Input::GetKeyPress(KEY_W) || Input::GetKeyPress(KEY_UP)) input.y += 1.f;
    if (Input::GetKeyPress(KEY_S) || Input::GetKeyPress(KEY_DOWN)) input.y -= 1.f;
    input = Math::Normalize(input);

    // 位置を直接書き換えず、PhysXの剛体に速度を渡す
    if (mode == DemoMode::Mode2D) {
        moverBody->SetVelocity({ input.x * moveSpeed, input.y * moveSpeed, 0.f });
        return;
    }

    // 3D: カメラの向き(yaw)を基準に、地面と水平に動く(Yは今の速度のまま)
    const DemoCameraController* orbit = camera ? camera->GetComponent<DemoCameraController>() : nullptr;
    const float yaw = orbit ? orbit->GetYaw() : 0.f;
    const float forwardX = sinf(yaw), forwardZ = cosf(yaw);
    const float rightX = cosf(yaw), rightZ = -sinf(yaw);
    moverBody->SetVelocity({ (forwardX * input.y + rightX * input.x) * moveSpeed, moverBody->GetVelocity().y,
                             (forwardZ * input.y + rightZ * input.x) * moveSpeed });
}

void TriggerDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 球を動かす
    MoveMover();

    // Rキー: 回数を0に戻す
    if (Input::GetKeyDown(KEY_R)) {
        if (zoneCounter) zoneCounter->ResetCounts();
        if (solidCounter) solidCounter->ResetCounts();
    }

    // トリガーの色: 球が中にいる間は濃くする
    const float alpha = (zoneCounter && zoneCounter->IsTouching()) ? zoneAlphaIn : zoneAlphaOut;
    if (SpriteRenderer* sprite = zone->GetComponent<SpriteRenderer>()) sprite->SetColor(1.f, 1.f, 1.f, alpha);
    if (MeshRenderer* mesh = zone->GetComponent<MeshRenderer>()) mesh->SetColor(1.f, 0.9f, 0.3f, alpha);
}

void TriggerDemoController::DrawCounts(GameObject* object, float halfHeight, const wchar_t* name, const wchar_t* counts) const {
    // 物の頭の上に、名前(上)と回数(下)を出す
    DirectX::XMFLOAT3 top = object->transform.position;
    top.y += halfHeight;
    DrawDemoLabel(mode, top, counts, 22.f);
    DrawDemoLabel(mode, top, name, 46.f);
}

void TriggerDemoController::Draw() {
    if (!zone || !solid || !zoneCounter || !solidCounter) return;

    const TouchCounterController::Counts& triggerCounts = zoneCounter->GetTrigger();
    const TouchCounterController::Counts& collisionCounts = solidCounter->GetCollision();

    // 物の上の名前と回数
    wchar_t buf[96];
    swprintf_s(buf, L"Enter %d / Stay %d / Exit %d", triggerCounts.enter, triggerCounts.stay, triggerCounts.exit);
    DrawCounts(zone, zone->transform.scale.y * 0.5f, L"トリガー(すり抜ける)", buf);
    swprintf_s(buf, L"Enter %d / Stay %d / Exit %d", collisionCounts.enter, collisionCounts.stay, collisionCounts.exit);
    DrawCounts(solid, solid->transform.scale.y * 0.5f, L"箱(ぶつかる)", buf);

    // 状態の一覧
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"黄色い丸(トリガー)  Enter %d  Stay %d  Exit %d", triggerCounts.enter, triggerCounts.stay, triggerCounts.exit);
    lines.push_back({ buf, yellow });
    swprintf_s(buf, L"青い箱(ぶつかる)  Enter %d  Stay %d  Exit %d", collisionCounts.enter, collisionCounts.stay, collisionCounts.exit);
    lines.push_back({ buf, blue });
    lines.push_back({ zoneCounter->IsTouching() ? L"球は今、トリガーの中に: いる" : L"球は今、トリガーの中に: いない", green });
    lines.push_back({ L"Stay は、中にいる間(ぶつかり続ける間)に毎フレーム呼ばれる", cyan });   // ゆっくり動かすと、Enter が1回で数えやすい
    lines.push_back({ L"[WASD・矢印] 球を動かす   [R] 回数を0に戻す", white });
    DrawDemoStatus(lines);
}
