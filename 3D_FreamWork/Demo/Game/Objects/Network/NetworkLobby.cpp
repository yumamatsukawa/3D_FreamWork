#include "NetworkLobby.h"
#include "../Common/DemoButton.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include <functional>
#include <vector>

namespace {
    constexpr float firstButtonX = -520.f;                   // 左上のボタンの中心のX
    constexpr float buttonStepX = 200.f;                     // ボタンの横の間隔
    constexpr float firstButtonY = 185.f;                    // 1行目のボタンの中心のY(説明の板の下)
    constexpr float buttonStepY = 45.f;                      // ボタンの縦の間隔
    constexpr int columns = 4;                               // 1行に並べるボタンの数
    const DirectX::XMFLOAT2 buttonSize = { 190.f, 40.f };    // ボタンの大きさ
    constexpr float buttonTextSize = 22.f;                   // ボタンの文字の大きさ
    constexpr int hostButtonCount = 4;                       // 見つかったホストのボタンの数
    constexpr float hostButtonX = 420.f;                     // 見つかったホストのボタンの中心のX(右の列)
    const DirectX::XMFLOAT2 hostButtonSize = { 400.f, 40.f };
}

GameObject* CreateNetworkLobby(Scene& scene, const NetworkLobbyOptions& options) {
    GameObject* lobby = scene.CreateObject("NetworkLobby");
    NetworkLobbyController* controller = lobby->AddComponent<NetworkLobbyController>(options);

    // 左上から並べるボタン(出さないボタンは詰める)
    struct Item { std::wstring label; std::function<void()> onClick; bool enabled; };
    const std::vector<Item> items = {
        { L"Host",           [controller] { controller->OnHost(); },         true },
        { L"Join 127.0.0.1", [controller] { controller->OnJoinLocal(); },    options.joinLocal },
        { L"Edit IP",        [controller] { controller->OnToggleEditIp(); }, options.joinIp },
        { L"Join IP",        [controller] { controller->OnJoinIp(); },       options.joinIp },
        { L"Find Hosts",     [controller] { controller->OnFind(); },         options.find },
        { L"Send",           [controller] { controller->OnSend(); },         options.send },
        { L"Disconnect",     [controller] { controller->OnDisconnect(); },   true },
    };
    int slot = 0;
    for (const Item& item : items) {
        if (!item.enabled) continue;
        CreateDemoButton(scene, item.label, { firstButtonX + buttonStepX * (float)(slot % columns), firstButtonY - buttonStepY * (float)(slot / columns) },
            buttonSize, buttonTextSize, item.onClick);
        slot++;
    }

    // 見つかったホストのボタン(右の列。ホストが見つかるまでは隠しておく)
    if (options.find) {
        for (int i = 0; i < hostButtonCount; i++) {
            GameObject* button = CreateDemoButton(scene, L"", { hostButtonX, firstButtonY - buttonStepY * (float)i },
                hostButtonSize, buttonTextSize, [controller, i] { controller->OnJoinFound(i); });
            button->SetActive(false);
            controller->AddHostButton(button);
        }
    }
    return lobby;
}