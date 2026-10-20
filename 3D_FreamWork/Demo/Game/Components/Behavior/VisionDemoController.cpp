#include "VisionDemoController.h"
#include "../Common/DemoBodyReset.h"
#include "../Common/DemoCameraController.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoPlane.h"
#include "../Common/DemoStatus.h"
#include "../../Objects/Behavior/DemoAgent.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/MathUtil.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Behavior/VisionComponent.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
    constexpr float turnSpeedManual = 2.0f;        // A・Dで回る速さ(ラジアン/秒)
    constexpr float turnSpeedAuto = 0.8f;          // 自動で回る速さ(ラジアン/秒)
    constexpr float fovStep = 15.f;                // 視野の角度を1回で変える量(度)
    constexpr float minFov = 15.f, maxFov = 360.f;
    constexpr float eyeHighOffset = 300.f;         // 3D版: 目を高くした時の高さ(壁の上より高く)

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 red = { 1.f, 0.35f, 0.3f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };
}

void VisionDemoController::Init() {
    // 扇形の線を出す(全体の設定。Uninit とガイドの ResetSettings で消す)
    VisionComponent::SetDebugDrawEnabled(true);
}

void VisionDemoController::Uninit() {
    VisionComponent::SetDebugDrawEnabled(false);
}

void VisionDemoController::Setup(GameObject* watcherObject, GameObject* targetObject, GameObject* cameraObject) {
    watcher = watcherObject;
    target = targetObject;
    camera = cameraObject;
    vision = watcher->GetComponent<VisionComponent>();
    targetBody = target->GetComponent<RigidbodyComponent>();
    targetStart = target->transform.position;

    // 2Dと3Dで、世界の大きさが違うので、距離とターゲットの速さも違う
    if (mode == DemoMode::Mode3D) {
        range = 1200.f;
        rangeStep = 150.f;
        targetSpeed = 400.f;
    }
    ApplyVision();
}

void VisionDemoController::ApplyVision() {
    vision->SetFieldOfView(fieldOfView);
    vision->SetRange(range);
    vision->SetObstacleTag(wallsBlock ? "Wall" : "");

    // 正面: 平面の上の向き(2DはXY、3DはXZ)。目の高さは、3D版だけ変えられる
    const float cosA = cosf(facingAngle), sinA = sinf(facingAngle);
    if (mode == DemoMode::Mode2D) {
        vision->SetFacing({ cosA, sinA, 0.f });
    }
    else {
        vision->SetFacing({ cosA, 0.f, sinA });
        vision->SetEyeOffset({ 0.f, eyeHigh ? eyeHighOffset : 0.f, 0.f });
    }
}

void VisionDemoController::Reset() {
    ResetDemoBody(target, targetStart);
    facingAngle = 0.f;
}

void VisionDemoController::MoveTarget() {
    if (!targetBody) return;

    // 入力の向き(右=x+、上/奥=y+)
    DirectX::XMFLOAT2 input = { 0.f, 0.f };
    if (Input::GetKeyPress(KEY_RIGHT)) input.x += 1.f;
    if (Input::GetKeyPress(KEY_LEFT)) input.x -= 1.f;
    if (Input::GetKeyPress(KEY_UP)) input.y += 1.f;
    if (Input::GetKeyPress(KEY_DOWN)) input.y -= 1.f;
    input = Math::Normalize(input);

    // 位置を直接書き換えず、PhysXの剛体に速度を渡す
    if (mode == DemoMode::Mode2D) {
        targetBody->SetVelocity({ input.x * targetSpeed, input.y * targetSpeed, 0.f });
        return;
    }

    // 3D: カメラの向き(yaw)を基準に、地面と水平に動く(Yは今の速度のまま)
    const DemoCameraController* orbit = camera ? camera->GetComponent<DemoCameraController>() : nullptr;
    const float yaw = orbit ? orbit->GetYaw() : 0.f;
    const float forwardX = sinf(yaw), forwardZ = cosf(yaw);
    const float rightX = cosf(yaw), rightZ = -sinf(yaw);
    targetBody->SetVelocity({ (forwardX * input.y + rightX * input.x) * targetSpeed, targetBody->GetVelocity().y,
                              (forwardZ * input.y + rightZ * input.x) * targetSpeed });
}

void VisionDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 見張り役の向き: A(左回り)・D(右回り)、SPACE で自動で回る
    if (Input::GetKeyPress(KEY_A)) facingAngle += turnSpeedManual * dt;
    if (Input::GetKeyPress(KEY_D)) facingAngle -= turnSpeedManual * dt;
    if (Input::GetKeyDown(KEY_SPACE)) spinning = !spinning;
    if (spinning) facingAngle += turnSpeedAuto * dt;

    // 視野の角度(W・S)・距離(1・2)・壁(5)・目の高さ(4。3Dだけ)・扇形の線(F1)
    if (Input::GetKeyDown(KEY_W)) fieldOfView = (std::min)(fieldOfView + fovStep, maxFov);
    if (Input::GetKeyDown(KEY_S)) fieldOfView = (std::max)(fieldOfView - fovStep, minFov);
    if (Input::GetKeyDown(KEY_2)) range = (std::min)(range + rangeStep, rangeStep * 10.f);
    if (Input::GetKeyDown(KEY_1)) range = (std::max)(range - rangeStep, rangeStep);
    if (Input::GetKeyDown(KEY_5)) wallsBlock = !wallsBlock;
    if (Input::GetKeyDown(KEY_4) && mode == DemoMode::Mode3D) eyeHigh = !eyeHigh;
    if (Input::GetKeyDown(KEY_F1)) {
        debugDraw = !debugDraw;
        VisionComponent::SetDebugDrawEnabled(debugDraw);
    }
    if (Input::GetKeyDown(KEY_R)) Reset();

    ApplyVision();
    MoveTarget();

    // 見えるか: 扇形の中か(角度と距離)、壁にさえぎられていないか(Raycast)
    inCone = vision->IsInCone(target->transform.position);
    seeing = vision->CanSee(target);

    // 見えた時は、見張り役が赤くなる
    SetDemoAgentCell(watcher, mode, seeing ? 0 : 6);
}

void VisionDemoController::Draw() {
    const bool is2D = (mode == DemoMode::Mode2D);

    // 目からターゲットへの線: 見えている時は赤、扇形の中だが壁にさえぎられている時は黄色
    if (inCone) {
        const DirectX::XMFLOAT3 eye = watcher->transform.position + DirectX::XMFLOAT3{ 0.f, (!is2D && eyeHigh) ? eyeHighOffset : 0.f, 0.f };
        const int dotCount = (int)Math::Clamp(Math::Distance(eye, target->transform.position) / (is2D ? 14.f : 30.f), 8.f, 120.f);
        DrawDemoLine(mode, eye, target->transform.position, seeing ? red : yellow, dotCount);
        if (!is2D && eyeHigh) DrawDemoPoint(mode, eye, 12.f, cyan);
    }

    // 名前
    DrawDemoLabel(mode, watcher->transform.position, L"見張り役", is2D ? 46.f : 56.f);
    DrawDemoLabel(mode, target->transform.position, L"ターゲット", is2D ? 40.f : 50.f);

    // 状態
    wchar_t buf[160];
    std::vector<DemoStatusLine> lines;
    swprintf_s(buf, L"視野の角度 SetFieldOfView: %.0f 度 (W S)   距離 SetRange: %.0f (1 2)", fieldOfView, range);
    lines.push_back({ buf, white });
    lines.push_back({ inCone ? L"ターゲットは 扇形の中にいる(IsInCone: はい)" : L"ターゲットは 扇形の外(IsInCone: いいえ)", cyan });
    lines.push_back({ !inCone ? L"扇形の外なので、壁は調べない → 見えない(CanSee: いいえ)"
                               : (seeing ? L"壁にさえぎられていない → 見える(CanSee: はい)" : L"壁にさえぎられている → 見えない(CanSee: いいえ)"),
                      seeing ? red : yellow });
    if (is2D) {
        swprintf_s(buf, L"壁 SetObstacleTag: %ls (5)", wallsBlock ? L"\"Wall\"(壁で見えなくなる)" : L"\"\"(壁を考えない)");
    }
    else {
        swprintf_s(buf, L"壁 SetObstacleTag: %ls (5)   目の高さ SetEyeOffset: %ls (4)", wallsBlock ? L"\"Wall\"" : L"なし", eyeHigh ? L"高い" : L"低い");
    }
    lines.push_back({ buf, green });
    lines.push_back({ L"矢印 ターゲット  A D 向き  SPACE 自動で回る  F1 扇形の線  R 戻す", white });
    DrawDemoStatus(lines, 740.f);
}
