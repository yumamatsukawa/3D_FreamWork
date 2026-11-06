#include "NetworkSpawnDemo.h"
#include "../Common/DemoButton.h"
#include "../../Components/Network/NetworkSpawnManager.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateNetworkSpawnDemo(Scene& scene, DemoMode mode) {
    // 弾を出す・消す管理役
    GameObject* manager = scene.CreateObject("NetworkSpawnManager");
    NetworkSpawnManager* controller = manager->AddComponent<NetworkSpawnManager>(mode == DemoMode::Mode3D);

    // 撃つボタン(SPACEキーでも撃てる)
    CreateDemoButton(scene, L"Fire (Space)", { -430.f, -130.f }, { 260.f, 50.f }, 24.f, [controller] { controller->Fire(); });
    return manager;
}