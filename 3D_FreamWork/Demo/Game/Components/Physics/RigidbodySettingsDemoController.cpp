#include "RigidbodySettingsDemoController.h"
#include "../Common/DemoBodyReset.h"
#include "../Common/DemoMarker.h"
#include "../Common/DemoStatus.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Core/Input.h"
#include "../../../../Engine/Core/Time.h"
#include "../../../../Engine/Physics/RigidbodyComponent.h"
#include <cstdio>
#include <string>
#include <vector>

namespace {
    constexpr float baseMass = 100.f;         // ふつうの質量(Bの質量変更は、この5倍)
    constexpr float heavyScale = 5.f;
    constexpr float highDrag = 3.f;           // 空気抵抗を大きくした時の値
    constexpr float kickSpeed2D = 400.f;      // 蹴った時に、ふつうの箱が受ける速さ(2D)
    constexpr float kickSpeed3D = 800.f;      // 同じ(3D。地面の摩擦で止まるので、2Dより速くする)
    constexpr float kickUpRatio = 0.25f;      // 蹴る向きの上向きの割合(横を1として)
    constexpr float panelWidth = 700.f;       // 状態の板の幅

    const DirectX::XMFLOAT4 white = { 1.f, 1.f, 1.f, 1.f };
    const DirectX::XMFLOAT4 yellow = { 1.f, 0.9f, 0.4f, 1.f };
    const DirectX::XMFLOAT4 green = { 0.5f, 1.f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 orange = { 1.f, 0.75f, 0.5f, 1.f };
    const DirectX::XMFLOAT4 cyan = { 0.6f, 0.9f, 1.f, 1.f };

    // 設定の名前と説明
    const wchar_t* GetSettingName(RigidbodySettingsDemoController::Setting setting) {
        using Setting = RigidbodySettingsDemoController::Setting;
        switch (setting) {
        case Setting::Mass:             return L"質量(SetMass)";
        case Setting::Drag:             return L"空気抵抗(SetDrag)";
        case Setting::Gravity:          return L"重力(SetUseGravity)";
        case Setting::FreezeRotation:   return L"回転固定(SetFreezeRotation)";
        default:                        return L"位置固定(SetFreezePositionX/Y/Z)";
        }
    }
    const wchar_t* GetSettingInfo(RigidbodySettingsDemoController::Setting setting, bool is2D) {
        using Setting = RigidbodySettingsDemoController::Setting;
        switch (setting) {
        case Setting::Mass:             return L"重い物は、同じ力で蹴っても動きにくい";
        case Setting::Drag:             return L"空気抵抗が大きいと、蹴ってもすぐ止まる";
        case Setting::Gravity:          return is2D ? L"2Dは重力が無いので、Bだけ重力を入れて落とす" : L"3Dは重力があるので、Bだけ重力を切って浮かせる";
        case Setting::FreezeRotation:   return L"回転を固定すると、蹴っても回らない";
        default:                        return L"位置を固定した向きには動かない。もう一度[5]で軸を変える";
        }
    }

    // 位置固定の軸の文字(なし / X / XY など)
    std::wstring GetFreezeText(bool x, bool y, bool z) {
        if (!x && !y && !z) return L"なし";
        std::wstring text;
        if (x) text += L"X";
        if (y) text += L"Y";
        if (z) text += L"Z";
        return text;
    }
}

RigidbodySettingsDemoController::State RigidbodySettingsDemoController::MakeDefaultState() const {
    // 2Dは重力が無く、Zを固定(奥に動かさない)。3Dは重力があって、どの軸も固定しない
    const bool is2D = (mode == DemoMode::Mode2D);
    return { baseMass, 0.f, !is2D, false, false, false, is2D };
}

void RigidbodySettingsDemoController::Apply(const Box& box, const State& state) {
    RigidbodyComponent* rigidbody = box.rigidbody;
    if (!rigidbody) return;
    rigidbody->SetMass(state.mass);
    rigidbody->SetDrag(state.drag);
    rigidbody->SetUseGravity(state.gravity);
    rigidbody->SetFreezeRotation(state.freezeRotation);
    rigidbody->SetFreezePositionX(state.freezeX);
    rigidbody->SetFreezePositionY(state.freezeY);
    rigidbody->SetFreezePositionZ(state.freezeZ);
}

void RigidbodySettingsDemoController::ApplySetting() {
    // Aは既定のまま、Bは選んだ設定だけ変える
    stateA = MakeDefaultState();
    stateB = stateA;
    switch (setting) {
    case Setting::Mass:             stateB.mass = baseMass * heavyScale; break;
    case Setting::Drag:             stateB.drag = highDrag; break;
    case Setting::Gravity:          stateB.gravity = !stateA.gravity; break;
    case Setting::FreezeRotation:   stateB.freezeRotation = true; break;
    case Setting::FreezePosition:
        if (freezeAxis == 0) stateB.freezeX = true;
        else if (freezeAxis == 1) stateB.freezeY = true;
        else stateB.freezeZ = true;
        break;
    }
    Apply(boxA, stateA);
    Apply(boxB, stateB);
    Reset();
}

void RigidbodySettingsDemoController::Setup(GameObject* normalBody, GameObject* changedBody) {
    boxA.object = normalBody;
    boxA.rigidbody = normalBody->GetComponent<RigidbodyComponent>();
    boxA.start = normalBody->transform.position;
    boxA.size = normalBody->transform.scale.x;
    boxB.object = changedBody;
    boxB.rigidbody = changedBody->GetComponent<RigidbodyComponent>();
    boxB.start = changedBody->transform.position;
    boxB.size = changedBody->transform.scale.x;
    ApplySetting();
}

void RigidbodySettingsDemoController::Reset() {
    ResetDemoBody(boxA.object, boxA.start);
    ResetDemoBody(boxB.object, boxB.start);
}

void RigidbodySettingsDemoController::Kick(const Box& box) {
    if (!box.rigidbody) return;
    // 同じ力を2つの箱に渡す: 右上へ蹴る(一瞬の衝撃)+回す(質量×大きさ×大きさ。ふつうの箱がちょうどよく回る強さ)
    const float kickSpeed = (mode == DemoMode::Mode2D) ? kickSpeed2D : kickSpeed3D;
    box.rigidbody->AddForce({ baseMass * kickSpeed, baseMass * kickSpeed * kickUpRatio, 0.f }, ForceMode::Impulse);
    box.rigidbody->AddTorque({ 0.f, 0.f, baseMass * boxA.size * boxA.size }, ForceMode::Impulse);
}

void RigidbodySettingsDemoController::Update(float dt) {
    // 切り替えの演出中は操作を受け付けない
    if (Time::IsPaused()) return;

    // 1〜5キー: Bに変える設定(5は、もう一度押すと固定する軸を変える)
    bool changed = false;
    if (Input::GetKeyDown(KEY_1)) { setting = Setting::Mass; changed = true; }
    if (Input::GetKeyDown(KEY_2)) { setting = Setting::Drag; changed = true; }
    if (Input::GetKeyDown(KEY_3)) { setting = Setting::Gravity; changed = true; }
    if (Input::GetKeyDown(KEY_4)) { setting = Setting::FreezeRotation; changed = true; }
    if (Input::GetKeyDown(KEY_5)) {
        if (setting == Setting::FreezePosition) freezeAxis = (freezeAxis + 1) % ((mode == DemoMode::Mode2D) ? 2 : 3);
        setting = Setting::FreezePosition;
        changed = true;
    }
    if (changed) ApplySetting();

    // SPACEキー: 2つの箱を同じ力で蹴る
    if (Input::GetKeyDown(KEY_SPACE)) {
        Kick(boxA);
        Kick(boxB);
    }

    // Rキー: 最初の位置へ戻す
    if (Input::GetKeyDown(KEY_R)) Reset();
}

void RigidbodySettingsDemoController::DrawBoxLabel(const Box& box, const wchar_t* name) const {
    if (!box.object) return;
    DirectX::XMFLOAT3 position = box.object->transform.position;
    position.y += box.size * 0.5f + ((mode == DemoMode::Mode2D) ? 22.f : 50.f);
    DrawDemoLabel(mode, position, name, 0.f);
}

void RigidbodySettingsDemoController::Draw() {
    // 箱の上の名札
    DrawBoxLabel(boxA, mode == DemoMode::Mode2D ? L"A ふつう" : L"A ふつう(手前)");
    DrawBoxLabel(boxB, mode == DemoMode::Mode2D ? L"B 変更" : L"B 変更(奥)");

    // 箱の設定を1行にまとめる
    auto describe = [](const wchar_t* name, const State& state) {
        wchar_t buf[160];
        swprintf_s(buf, L"%ls 質量%.0f 抵抗%.0f 重力%ls 回転固定:%ls 位置固定:%ls", name, state.mass, state.drag,
            state.gravity ? L"あり" : L"なし", state.freezeRotation ? L"あり" : L"なし",
            GetFreezeText(state.freezeX, state.freezeY, state.freezeZ).c_str());
        return std::wstring(buf);
    };

    // 状態の一覧
    std::vector<DemoStatusLine> lines;
    lines.push_back({ L"[1]質量 [2]抵抗 [3]重力 [4]回転固定 [5]位置固定", white });
    lines.push_back({ std::wstring(L"Bに変えた設定: ") + GetSettingName(setting), yellow });
    lines.push_back({ GetSettingInfo(setting, mode == DemoMode::Mode2D), cyan });
    lines.push_back({ describe(L"A:", stateA), green });
    lines.push_back({ describe(L"B:", stateB), orange });
    lines.push_back({ L"SPACE: 同じ力で蹴る(Impulse+AddTorque)  R: 元に戻す", white });
    DrawDemoStatus(lines, panelWidth);
}
