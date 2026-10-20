#include "ResourcesDemo.h"
#include "../Common/DemoButton.h"
#include "../../Components/Core/ResourcesDemoController.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"

GameObject* CreateResourcesDemo(Scene& scene) {
    // 画像・音・モデルの借り入れを持つ役(ボタンが押されたら、この役に頼む)
    GameObject* demo = scene.CreateObject("ResourcesDemo");
    ResourcesDemoController* controller = demo->AddComponent<ResourcesDemoController>();

    // ボタン2列x3行(左: 画像 / 右: 音・モデル・解放)
    const DirectX::XMFLOAT2 size = { 190.f, 48.f };
    CreateDemoButton(scene, L"Add 10", { -520.f, -20.f }, size, 26.f, [controller] { controller->AddSprites(10); });
    CreateDemoButton(scene, L"Remove", { -520.f, -80.f }, size, 26.f, [controller] { controller->DestroySprites(); });
    CreateDemoButton(scene, L"Unload", { -520.f, -140.f }, size, 26.f, [controller] { controller->UnloadUnused(); });
    CreateDemoButton(scene, L"Sound x2", { -300.f, -20.f }, size, 26.f, [controller] { controller->LoadSounds(); });
    CreateDemoButton(scene, L"Model x2", { -300.f, -80.f }, size, 26.f, [controller] { controller->LoadModels(); });
    CreateDemoButton(scene, L"Release", { -300.f, -140.f }, size, 26.f, [controller] { controller->ReleaseHeld(); });
    return demo;
}
