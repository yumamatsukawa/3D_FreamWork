#include "ForceDemoController.h"
#include "../Common/DemoBodyReset.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <cmath>
#include <cstdio>

namespace {
    constexpr float lightMass = 100.f;        // 軽い箱の質量
    constexpr float heavyMass = 500.f;        // 重い箱の質量(軽い箱の5倍)
    constexpr float boxDrag = 0.6f;           // 空気抵抗(押すのをやめると、だんだん止まる)
    constexpr float forceAcceleration = 1500.f;   // Force・Acceleration: 押し続ける強さ(軽い箱が受ける加速度)
    constexpr float kickSpeed = 350.f;        // Impulse・VelocityChange・SetVelocity: 一瞬で付ける速さ(軽い箱が受ける速さ)

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };

    // 力の種類の名前と説明
    const wchar_t* GetForceModeName(ForceMode mode) {
        switch (mode) {
        case ForceMode::Force:          return L"Force";
        case ForceMode::Impulse:        return L"Impulse";
        case ForceMode::Acceleration:   return L"Acceleration";
        default:                        return L"VelocityChange";
        }
    }
    const wchar_t* GetForceModeInfo(ForceMode mode) {
        switch (mode) {
        case ForceMode::Force:          return L"押し続ける力。重い物ほど動きにくい(SPACEを押している間)";
        case ForceMode::Impulse:        return L"一瞬の衝撃。重い物ほど動きにくい(SPACEを押すたびに1回)";
        case ForceMode::Acceleration:   return L"押し続ける加速度。重さは関係ない(SPACEを押している間)";
        default:                        return L"一瞬の速度の変化。重さは関係ない(SPACEを押すたびに1回)";
        }
    }
}

void ForceDemoController::SetupBox(Box& box, GameObject* object, float mass) {
    box.object = object;
    box.rigidbody = object->GetComponent<RigidbodyComponent>();
    box.start = object->transform.position;
    box.size = object->transform.scale.x;
    box.mass = mass;
    if (!box.rigidbody) return;

    // 質量を決める(作った時は「密度×体積」なので、比べやすい値に直す)・空気抵抗を付ける
    box.rigidbody->SetMass(mass);
    box.rigidbody->SetDrag(boxDrag);

    // 横にだけ動くよう、YとZを固定する(2Dは作る時にZを固定済み。3Dは重力も切って、力の違いだけを見る)
    box.rigidbody->SetFreezePositionY(true);
    box.rigidbody->SetFreezePositionZ(true);
    box.rigidbody->SetUseGravity(false);
}

void ForceDemoController::Setup(GameObject* lightBody, GameObject* heavyBody) {
    SetupBox(light, lightBody, lightMass);
    SetupBox(heavy, heavyBody, heavyMass);
}

void ForceDemoController::Reset() {
    ResetDemoBody(light.object, light.start);
    ResetDemoBody(heavy.object, heavy.start);
}

void ForceDemoController::Push(const Box& box) {
    if (!box.rigidbody) return;
    // どの種類でも、軽い箱が同じ加速度・同じ速さになる大きさで、同じ値を2つの箱に渡す(力は質量をかけた値)
    switch (forceMode) {
    case ForceMode::Force:
        box.rigidbody->AddForce({ lightMass * forceAcceleration, 0.f, 0.f }, ForceMode::Force);
        break;
    case ForceMode::Impulse:
        box.rigidbody->AddForce({ lightMass * kickSpeed, 0.f, 0.f }, ForceMode::Impulse);
        break;
    case ForceMode::Acceleration:
        box.rigidbody->AddForce({ forceAcceleration, 0.f, 0.f }, ForceMode::Acceleration);
        break;
    case ForceMode::VelocityChange:
        box.rigidbody->AddForce({ kickSpeed, 0.f, 0.f }, ForceMode::VelocityChange);
        break;
    }
}

void ForceDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1〜4キー: 力の種類
    if (Input::GetKeyDown(KEY_1)) forceMode = ForceMode::Force;
    if (Input::GetKeyDown(KEY_2)) forceMode = ForceMode::Impulse;
    if (Input::GetKeyDown(KEY_3)) forceMode = ForceMode::Acceleration;
    if (Input::GetKeyDown(KEY_4)) forceMode = ForceMode::VelocityChange;

    // SPACEキー: 力をかける(押し続ける種類は、押している間ずっと。一瞬の種類は、押すたびに1回)
    const bool continuous = (forceMode == ForceMode::Force || forceMode == ForceMode::Acceleration);
    if (continuous ? Input::GetKeyPress(KEY_SPACE) : Input::GetKeyDown(KEY_SPACE)) {
        Push(light);
        Push(heavy);
    }

    // Vキー: 速さを決めた値に「上書き」する(今の速さは関係ない。AddForceは今の速さに足される)
    if (Input::GetKeyDown(KEY_V)) {
        if (light.rigidbody) light.rigidbody->SetVelocity({ kickSpeed, 0.f, 0.f });
        if (heavy.rigidbody) heavy.rigidbody->SetVelocity({ kickSpeed, 0.f, 0.f });
    }

    // Tキー: 回す力(AddTorque)。同じ大きさの力で、重い箱ほど回りにくい
    if (Input::GetKeyDown(KEY_T) && light.rigidbody && heavy.rigidbody) {
        const float torque = light.mass * light.size * light.size;
        light.rigidbody->AddTorque({ 0.f, 0.f, torque }, ForceMode::Impulse);
        heavy.rigidbody->AddTorque({ 0.f, 0.f, torque }, ForceMode::Impulse);
    }

    // Rキー: 最初の位置へ戻す
    if (Input::GetKeyDown(KEY_R)) Reset();
}

void ForceDemoController::DrawBoxLabel(const Box& box, const wchar_t* name) const {
    if (!box.object || !box.rigidbody) return;

    // 箱の上に、名前・質量・今の速さ(GetVelocity)を出す
    wchar_t buf[96];
    swprintf_s(buf, L"%ls 質量%.0f 速さ%.0f", name, box.mass, fabsf(box.rigidbody->GetVelocity().x));
    DirectX::XMFLOAT3 position = box.object->transform.position;
    position.y += box.size * 0.5f + ((mode == DemoMode::Mode2D) ? 22.f : 50.f);
    DrawDemoLabel(mode, position, buf, 0.f);
}

void ForceDemoController::Draw() {
    // 箱の上の名札
    DrawBoxLabel(light, L"軽い箱");
    DrawBoxLabel(heavy, L"重い箱");

    // 状態の一覧
    wchar_t buf[96];
    std::vector<DemoStatusLine> lines;
    lines.push_back({ std::wstring(L"[1]〜[4] 力の種類: ") + GetForceModeName(forceMode), yellow });
    lines.push_back({ GetForceModeInfo(forceMode), white });
    swprintf_s(buf, L"[V] SetVelocityで速さを%.0fにする(上書き)  [T] AddTorqueで回す", kickSpeed);
    lines.push_back({ buf, cyan });
    swprintf_s(buf, L"軽い箱: 質量 %.0f / 速さ %.0f", light.mass, light.rigidbody ? fabsf(light.rigidbody->GetVelocity().x) : 0.f);
    lines.push_back({ buf, green });
    swprintf_s(buf, L"重い箱: 質量 %.0f / 速さ %.0f", heavy.mass, heavy.rigidbody ? fabsf(heavy.rigidbody->GetVelocity().x) : 0.f);
    lines.push_back({ buf, green });
    lines.push_back({ L"[R] 元の位置へ戻す  (箱は横にだけ動く)", white });
    DrawDemoStatus(lines);
}
