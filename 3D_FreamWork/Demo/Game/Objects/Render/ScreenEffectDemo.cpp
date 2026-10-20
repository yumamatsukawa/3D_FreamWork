#include "ScreenEffectDemo.h"
#include "../../Components/Render/ScreenEffectDemoController.h"
#include "../../Components/Core/SpinComponent.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"

GameObject* CreateScreenEffectDemo(Scene& scene) {
    // 演出が見えやすいよう、回る箱を横に並べる
    for (int i = 0; i < 5; i++) {
        GameObject* box = scene.CreateObject("Box");
        box->transform.position = { -400.f + 200.f * (float)i, -110.f, 0.f };
        box->transform.scale = { 110.f, 110.f, 1.f };
        box->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(i * 2);
        box->AddComponent<SpinComponent>(DemoMode::Mode2D, 60.f);
    }

    // 演出を出す操作役
    GameObject* demo = scene.CreateObject("ScreenEffectDemo");
    demo->AddComponent<ScreenEffectDemoController>();
    return demo;
}
