#include "NetworkDoorController.h"
#include "../Common/DemoTextStyle.h"
#include "../../../../Engine/Core/EventBus.h"
#include "../../../../Engine/Network/Network.h"
#include "../../../../Engine/Render/Image.h"
#include "../../../../Engine/UI/Text.h"
#include <algorithm>

namespace {
    constexpr int maxLog = 5;                                        // 覚えておく記録の数
    constexpr float openSpeed = 2.5f;                                // 扉が動く速さ(1秒あたりの開き具合)
    const DirectX::XMFLOAT2 doorCenter = { 180.f, -170.f };          // 扉の枠の中心(画面座標)
    const DirectX::XMFLOAT2 doorSize = { 160.f, 260.f };             // 扉の枠の大きさ
    const DirectX::XMFLOAT2 lampA = { -230.f, -110.f };              // スイッチAのランプ
    const DirectX::XMFLOAT2 lampB = { -230.f, -170.f };              // スイッチBのランプ
    constexpr float lampSize = 36.f;
    const DirectX::XMFLOAT2 logPosition = { 330.f, -60.f };          // 記録の1行目(左端)
}

void NetworkDoorController::AddLog(const std::wstring& text) {
    log.insert(log.begin(), text);
    if ((int)log.size() > maxLog) log.pop_back();
}

void NetworkDoorController::PressSwitch(int index) {
    // 押したことをホストに知らせる(ホスト自身が押した時も、同じ道を通る)
    NetworkWriter payload;
    payload.WriteU8((uint8_t)index);
    Network::SendEvent("SwitchPressed", payload, NetworkTarget::Host);
}

void NetworkDoorController::ResetDoor() {
    Network::SendEvent("ResetDoor", NetworkTarget::Host);
}

void NetworkDoorController::Init() {
    // ─ ホストだけが判断する部分(スイッチを数えて、扉を開ける) ─
    Network::Subscribe("SwitchPressed", [this](NetworkReader& payload, int sender) {
        const int index = payload.ReadU8();
        if (!payload.IsOk() || index < 0 || index > 1 || !Network::IsHost()) return;
        AddLog(L"スイッチ" + std::wstring(index == 0 ? L"A" : L"B") + L"が押された(プレイヤー" + std::to_wstring(sender) + L")");

        // 今の状態は共有の値に入れる(途中参加の人にも渡る)
        Network::SetSharedValue(index == 0 ? "SwitchA" : "SwitchB", 1);
        if (Network::GetSharedValue("SwitchA") == 1 && Network::GetSharedValue("SwitchB") == 1 && Network::GetSharedValue("Door") == 0) {
            Network::SetSharedValue("Door", 1);
            Network::SendEvent("DoorOpened", NetworkTarget::All);   // 開いた瞬間の演出用(全員へ)
        }
    });
    Network::Subscribe("ResetDoor", [this](NetworkReader&, int) {
        if (!Network::IsHost()) return;
        Network::SetSharedValue("SwitchA", 0);
        Network::SetSharedValue("SwitchB", 0);
        Network::SetSharedValue("Door", 0);
        Network::SendEvent("DoorClosed", NetworkTarget::All);
    });

    // ─ 全員で受ける演出の部分(中身の無い出来事は、EventBus でも受けられる) ─
    EventBus::Get().Subscribe("DoorOpened", [this]() { AddLog(L"DoorOpened を受け取った(扉が開いた)"); });
    EventBus::Get().Subscribe("DoorClosed", [this]() { AddLog(L"DoorClosed を受け取った(扉が閉じた)"); });

    // 途中参加した時、すでに開いている扉は最初から開いた見た目にする
    openAmount = Network::GetSharedValue("Door") == 1 ? 1.f : 0.f;
}

void NetworkDoorController::Update(float dt) {
    // 扉の見た目は、共有の値(今の状態)に向かって動く。途中参加でも、値が届けば開いた見た目になる
    const float target = Network::GetSharedValue("Door") == 1 ? 1.f : 0.f;
    if (openAmount < target) openAmount = (std::min)(target, openAmount + openSpeed * dt);
    else if (openAmount > target) openAmount = (std::max)(target, openAmount - openSpeed * dt);
}

void NetworkDoorController::Draw() {
    // 扉の奥(開くと緑に光る)と枠
    const bool open = openAmount > 0.f;
    Image::DrawRect(doorCenter, { doorSize.x + 24.f, doorSize.y + 24.f }, { 0.35f, 0.35f, 0.4f, 1.f });
    Image::DrawRect(doorCenter, doorSize, open ? DirectX::XMFLOAT4{ 0.3f, 0.9f, 0.4f, 1.f } : DirectX::XMFLOAT4{ 0.05f, 0.05f, 0.08f, 1.f });

    // 扉の板: 上へ上がっていく(上の端がそろう)
    const float slabHeight = doorSize.y * (1.f - openAmount);
    if (slabHeight > 0.5f) {
        Image::DrawRect({ doorCenter.x, doorCenter.y + doorSize.y * 0.5f - slabHeight * 0.5f }, { doorSize.x, slabHeight }, { 0.6f, 0.45f, 0.3f, 1.f });
    }

    // スイッチのランプ(共有の値)
    const bool a = Network::GetSharedValue("SwitchA") == 1;
    const bool b = Network::GetSharedValue("SwitchB") == 1;
    Image::DrawRect(lampA, { lampSize, lampSize }, a ? DirectX::XMFLOAT4{ 1.f, 0.9f, 0.2f, 1.f } : DirectX::XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.f });
    Image::DrawRect(lampB, { lampSize, lampSize }, b ? DirectX::XMFLOAT4{ 1.f, 0.9f, 0.2f, 1.f } : DirectX::XMFLOAT4{ 0.2f, 0.2f, 0.2f, 1.f });

    // 文字: 扉の状態と、受け取った出来事の記録
    Text::Draw(open && openAmount >= 1.f ? L"扉: 開いている" : (open ? L"扉: 開いていく..." : L"扉: 閉じている(スイッチを両方押す)"),
        doorCenter.x - 90.f, doorCenter.y - doorSize.y * 0.5f - 34.f, MakeDemoTextStyle(19.f, { 1.f, 0.9f, 0.4f, 1.f }));
    Text::Draw(L"受け取った出来事:", logPosition.x, logPosition.y, MakeDemoTextStyle(19.f, { 0.6f, 0.9f, 1.f, 1.f }));
    for (size_t i = 0; i < log.size(); i++) {
        Text::Draw(log[i], logPosition.x, logPosition.y - 26.f * (float)(i + 1), MakeDemoTextStyle(17.f));
    }
}