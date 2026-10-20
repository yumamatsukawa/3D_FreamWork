#include "TransitionDemo.h"
#include "../../Components/Core/SpinComponent.h"
#include "../../../../Engine/Core/Scene.h"
#include "../../../../Engine/Core/GameObject.h"
#include "../../../../Engine/Render/SpriteRenderer.h"
#include "../../../../Engine/Render3D/MeshRenderer.h"

GameObject* CreateTransitionDemo(Scene& scene, TransitionPage page) {
    // 回る箱(切り替え中は、ゲームの時間が止まるので、箱も止まる)
    for (int i = 0; i < 3; i++) {
        GameObject* box = scene.CreateObject("Box");
        if (page == TransitionPage::A) {
            box->transform.position = { -300.f + 300.f * (float)i, -130.f, 0.f };
            box->transform.scale = { 120.f, 120.f, 1.f };
            box->AddComponent<SpriteRenderer>(L"Assets/box.png", 3, 3)->SetSpriteIndex(i * 3);
            box->AddComponent<SpinComponent>(DemoMode::Mode2D, 90.f);
        }
        else {
            box->transform.position = { -250.f + 250.f * (float)i, 0.f, 300.f };
            box->transform.scale = { 100.f, 100.f, 100.f };
            box->AddComponent<MeshRenderer>(Mesh::CreateCube())->SetColor(1.0f - 0.3f * (float)i, 0.6f, 0.3f + 0.3f * (float)i, 1.0f);
            box->AddComponent<SpinComponent>(DemoMode::Mode3D, 90.f);
        }
    }

    // 演出を選ぶ操作役
    GameObject* demo = scene.CreateObject("TransitionDemo");
    demo->AddComponent<TransitionDemoController>(page);
    return demo;
}
