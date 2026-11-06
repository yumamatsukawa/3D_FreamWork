#include "NetworkEventDemo.h"
#include "../Common/DemoButton.h"
#include "../../Components/Network/NetworkDoorController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

namespace {
    const DirectX::XMFLOAT2 buttonSize = { 260.f, 50.f };
    constexpr float buttonX = -400.f;
    constexpr float buttonTextSize = 24.f;
}

GameObject* CreateNetworkEventDemo(Scene& scene) {
    // 扉の見た目と、出来事を受けて判断する役
    GameObject* door = scene.CreateObject("NetworkDoor");
    NetworkDoorController* controller = door->AddComponent<NetworkDoorController>();

    // スイッチ2つ(両方押すと開く)と、元に戻すボタン
    CreateDemoButton(scene, L"Press Switch A", { buttonX, -110.f }, buttonSize, buttonTextSize, [controller] { controller->PressSwitch(0); });
    CreateDemoButton(scene, L"Press Switch B", { buttonX, -170.f }, buttonSize, buttonTextSize, [controller] { controller->PressSwitch(1); });
    CreateDemoButton(scene, L"Reset Door", { buttonX, -230.f }, buttonSize, buttonTextSize, [controller] { controller->ResetDoor(); });
    return door;
}